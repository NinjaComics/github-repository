/* -*- c++ -*- */
/* 
 * Copyright 2013 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "ofdm_carrier_allocator_cvc_impl.h"
#include "fstream"
#include "iostream"
#include "algorithm"

namespace gr {
  namespace howto {

    ofdm_carrier_allocator_cvc::sptr
    ofdm_carrier_allocator_cvc::make(
	int num_user,
	float total_power,
	int bandwidth,
	float noise_density,
	int fft_len,
	const std::vector<std::vector<int> > &occupied_carriers,
	const std::vector<std::vector<int> > &pilot_carriers,
	const std::vector<std::vector<gr_complex> > &pilot_symbols,
	const std::vector<std::vector<gr_complex> > &sync_words,
	const std::string &len_tag_key,
	const bool output_is_shifted
    )
    {
      return gnuradio::get_initial_sptr(
	  new ofdm_carrier_allocator_cvc_impl(
	    num_user,
	    total_power,
	    bandwidth,
	    noise_density,
	    fft_len,
	    occupied_carriers,
	    pilot_carriers,
	    pilot_symbols,
	    sync_words,
	    len_tag_key,
	    output_is_shifted
	  )
      );
    }

 


    ofdm_carrier_allocator_cvc_impl::ofdm_carrier_allocator_cvc_impl(
	  int num_user,
	  float total_power,
	  int bandwidth,
	  float noise_density,
	  int fft_len,
	  const std::vector<std::vector<int> > &occupied_carriers,
	  const std::vector<std::vector<int> > &pilot_carriers,
	  const std::vector<std::vector<gr_complex> > &pilot_symbols,
	  const std::vector<std::vector<gr_complex> > &sync_words,
	  const std::string &len_tag_key,
	  const bool output_is_shifted
    ) : tagged_stream_block("ofdm_carrier_allocator_cvc",
		   io_signature::make(1, 1, sizeof (gr_complex)),
		   io_signature::make(1, 1, sizeof (gr_complex) * fft_len), len_tag_key),
	d_total_power(total_power),
	d_bandwidth(bandwidth),
	d_noise_density(noise_density),
	d_fft_len(fft_len),

	d_temp1(0),
	d_temp2(0),
	d_num_user(num_user),
	d_occupied_carriers(occupied_carriers),
	//d_n_input(0),
	
	d_pilot_carriers(pilot_carriers),
	d_pilot_symbols(pilot_symbols),
	d_sync_words(sync_words),
	d_symbols_per_set(0),
	d_output_is_shifted(output_is_shifted)
    {
      for (unsigned i = 0; i < d_occupied_carriers.size(); i++) {
	for (unsigned j = 0; j < d_occupied_carriers[i].size(); j++) {
	  if (occupied_carriers[i][j] < 0) {
	    d_occupied_carriers[i][j] += d_fft_len;
	  }
	  if (d_occupied_carriers[i][j] > d_fft_len || d_occupied_carriers[i][j] < 0) {
	    throw std::invalid_argument("data carrier index out of bounds");
	  }
	  if (d_output_is_shifted) {
	    d_occupied_carriers[i][j] = (d_occupied_carriers[i][j] + fft_len/2) % fft_len;
	  }
	}
      }
      for (unsigned i = 0; i < d_pilot_carriers.size(); i++) {
	if (d_pilot_carriers[i].size() != pilot_symbols[i].size()) {
	  throw std::invalid_argument("pilot_carriers do not match pilot_symbols");
	}
	for (unsigned j = 0; j < d_pilot_carriers[i].size(); j++) {
	  if (d_pilot_carriers[i][j] < 0) {
	    d_pilot_carriers[i][j] += d_fft_len;
	  }
	  if (d_pilot_carriers[i][j] > d_fft_len || d_pilot_carriers[i][j] < 0) {
	    throw std::invalid_argument("pilot carrier index out of bounds");
	  }
	  if (d_output_is_shifted) {
	    d_pilot_carriers[i][j] = (d_pilot_carriers[i][j] + fft_len/2) % fft_len;
	  }
	}
      }
      for (unsigned i = 0; i < d_sync_words.size(); i++) {
	if (d_sync_words[i].size() != (unsigned) d_fft_len) {
	  throw std::invalid_argument("sync words must be fft length");
	}
      }

      for (unsigned i = 0; i < d_occupied_carriers.size(); i++) {
	d_symbols_per_set += d_occupied_carriers[i].size();

      }
      set_tag_propagation_policy(TPP_DONT);
      set_relative_rate((double) d_symbols_per_set / d_occupied_carriers.size());

      d_temp1=std::vector<int>(d_num_user);//subcarrier user allocated to
      d_temp2=std::vector<int>(d_num_user);//user id

      d_constellation = gr::howto::constellation_bpsk::make();
      d_symbol_table = d_constellation->s_points();
    }

    ofdm_carrier_allocator_cvc_impl::~ofdm_carrier_allocator_cvc_impl()
    {
    }

    //reset
    void
    ofdm_carrier_allocator_cvc_impl::reset(std::vector<power> *ptr_init_power_allo, std::vector<user> *ptr_usr)
    {
	//resource allocation initialization
	std::vector<power> a(d_fft_len);
	for (int i=0;i<d_fft_len;i++){
	    a[i].available = 1;
	    a[i].index= i;
	}
	* ptr_init_power_allo=a;

	std::vector<user> b(d_num_user);
	for (int i=0;i<b.size();i++){
	    b[i].allocated = 0;
	    b[i].index = i;
	}
	* ptr_usr = b;
    }

    //compare function for down sort
    bool compare(power a, power b)
    {
        power* power_a= &a;
	power* power_b= &b;
	if (power_a->value > power_b->value) return true;
        if (power_a->value < power_b->value) return false;
        return false;
    }

    //genrerate rayleigh random numbers
    std::vector<float> rayleigh_random (int num)
    {
	srand (2);

	std::vector<float> ran(num);
	std::vector<float> rayleigh(num);
	for(int i=0;i<num;i++)
        {
            ran[i] = (float)rand();
	    rayleigh[i] = ran[i] / RAND_MAX; 
	    rayleigh[i]=1/0.6552f*sqrt(-2*log(rayleigh[i]));
        }
	return rayleigh;
    }

    int
    ofdm_carrier_allocator_cvc_impl::calculate_output_stream_length(const gr_vector_int &ninput_items)
    {
      /*int nin = ninput_items[0];
      int nout = (nin / d_symbols_per_set) * d_occupied_carriers.size();
      int k = 0;
      for (int i = 0; i < nin % d_symbols_per_set; k++) {
	nout++;
	i += d_occupied_carriers[k % d_occupied_carriers.size()].size();

      }*/
    	int nout=30;
      return nout + d_sync_words.size();
    }

    bool
	ofdm_carrier_allocator_cvc_impl::check_existence(unsigned device_id){
    	if(!d_device_buffer.size()){
    		return false;
    	}else{
    		for(unsigned i=0;i<d_device_buffer.size();i++){
    			if(device_id==d_device_buffer[i])
    				return true;
    		}
    		return false;
    	}
    }

    int
    ofdm_carrier_allocator_cvc_impl::work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      std::vector<tag_t> tags;
      get_tags_in_range(tags, 0,
		  nitems_read(0),
		  nitems_read(0)+(int)ninput_items[0]
	  );

      long device_id=0;
      int outer_header_len = 6;
      std::vector<int> occupied_carriers;
      gr_complex *in_buffer;

      for(unsigned i=0;i<tags.size();i++){
    	  if(pmt::equal(tags[i].key, pmt::string_to_symbol("device_id")))
    		  device_id=pmt::to_long(tags[i].value);
    	  if(pmt::equal(tags[i].key, pmt::string_to_symbol("occupied_carriers")))
			  occupied_carriers=pmt::s32vector_elements(tags[i].value);
      }
      if(device_id==0 && d_device_buffer.size()==0){
    	  return 0;
      }else{

		  int size=sizeof(gr_complex) * ninput_items[0];
		  if(device_id!=0 && !check_existence(device_id)){
			  d_device_buffer.push_back(device_id);
			  d_carriers_buffer.push_back(occupied_carriers);

			  in_buffer =(gr_complex*)malloc(size);
			  memcpy( in_buffer,  (const gr_complex *) input_items[0], size);
			  d_in_buffer.push_back(in_buffer);
			  d_size.push_back(size/sizeof(gr_complex));
			  //d_n_input+=ninput_items[0];

			  //consume_each(ninput_items[0]);
			  return 0;
		  }else{



			//*******************************//
			//        waterfilling           //
			//*******************************//
			//initial
			/*std::vector<power> init_power_allo;
			std::vector<user> usr;
			reset(&init_power_allo, &usr);
	

			std::vector<float> channel_state_info = rayleigh_random(d_fft_len);

			//initial power allocation
			float NCR_sum=0; //sum of noise to carrier ratio
			float subchannel_noise = d_noise_density*d_bandwidth/d_fft_len;
			std::vector<float> carrier_to_noise_ratio(d_fft_len);
	
			for (int i = 0; i < d_fft_len; i++)
			{
				carrier_to_noise_ratio[i]=channel_state_info[i]*channel_state_info[i]/subchannel_noise;
				NCR_sum= NCR_sum+1/carrier_to_noise_ratio[i];
			}
			for (int i = 0; i < d_fft_len; i++)
			{
				init_power_allo[i].value=(d_total_power + NCR_sum)/d_fft_len - 1/carrier_to_noise_ratio[i];
				init_power_allo[i].NCR=1/carrier_to_noise_ratio[i];
				init_power_allo[i].index=i;
			}

			//iterative part of the algorithm
			//negtive check
			bool is_negtive=false;
			for(int i=0;i<d_fft_len;i++)
			{
				if(init_power_allo[i].value<0)
					is_negtive=true;
			}

			//iterative calculation
			while(is_negtive)
			{
				float NCR_rem_sum=0;
				int num_subchannel_rem=0;
				std::vector<float> CRN_rem(d_fft_len,0);
				for (int i=0;i<d_fft_len;i++)
				{
					if(init_power_allo[i].value>0)
					{
						num_subchannel_rem++;
						CRN_rem[i]=carrier_to_noise_ratio[i];
						NCR_rem_sum=NCR_rem_sum+1/CRN_rem[i];
					}
					else
						init_power_allo[i].value=0;
				}
				for (int i=0;i<d_fft_len;i++)
				{
					if(init_power_allo[i].value>0)
					{
						init_power_allo[i].value=(d_total_power + NCR_rem_sum)/num_subchannel_rem - 1/CRN_rem[i];
						init_power_allo[i].NCR=1/CRN_rem[i];
					}
				}
				//negtive check
				int num_negtive=0;
				for(int i=0;i<d_fft_len;i++)
				{
					if(init_power_allo[i].value<0)
						num_negtive++;
				}
				if(num_negtive==0)
					is_negtive=false;
			}
	
			std::ofstream outfile1 ("/home/chk/Documents/resource_allocation.txt");
			for (int i = 0; i < d_fft_len; i++){
				outfile1 << init_power_allo[i].index << "   "
					 << init_power_allo[i].value << "   "
					 <<init_power_allo[i].NCR <<"   "
					 <<init_power_allo[i].value+init_power_allo[i].NCR
					 <<std::endl;
			}
			outfile1.close();

			//disable pilot carriers
			for (int i = 0; i < d_pilot_carriers[0].size(); i++){
				init_power_allo[d_pilot_carriers[0][i]].available = 0;
			}

			//sort
			std::sort(init_power_allo.begin(), init_power_allo.end(), compare);*/

			//*************************//
			//      allocate user      //
			//*************************//
			/*std::ofstream outfile2 ("/home/chk/Documents/subchannel_status.txt");
			for (int i = 0; i < usr.size(); i++){

				for(int j = 0; j < d_fft_len; j++){
					if(init_power_allo[j].value!=0&&usr[i].allocated!=1&&init_power_allo[j].available==1){
						d_temp1[i]=init_power_allo[j].index;
						d_temp2[i]=usr[i].index;
						init_power_allo[j].available=0;
						usr[i].allocated =1;
					}
				}
				outfile2 << d_temp1[i] << "   " << d_temp2[i] <<"   \n";
			}
			for (int i = 0; i < d_pilot_carriers[0].size(); i++){
				outfile2 << d_pilot_carriers[0][i] << "   " << -1 << std::endl;
			}
			outfile2.close();*/
			//d_occupied_carriers.erase(d_occupied_carriers.begin());
			//d_occupied_carriers.push_back(d_temp1);



			  gr_complex *out = (gr_complex *) output_items[0];
			  memset((void *) out, 0x00, sizeof(gr_complex) * d_fft_len * noutput_items);

			  // Copy Sync word
			  for (unsigned i = 0; i < d_sync_words.size(); i++) {
				memcpy((void *) out, (void *) &d_sync_words[i][0], sizeof(gr_complex) * d_fft_len);
				out += d_fft_len;
			  }

			  //add outer header
			  char pid[]= {7,11,13,14,19,21,22,25,26,28,15,60,58,57,3,5,6,9,10,12,17,18,20,24,33,34,36,40,48,62,61,59,55}; // pid with 2 or 3 or 4 ones
			  for(int i=0;i<d_device_buffer.size();i++){
				  for(int j=0;j<d_carriers_buffer[i].size();j++){
					  for(int t=0;t<outer_header_len;t++){
						  out[d_carriers_buffer[i][j] + t * d_fft_len]= d_symbol_table[(pid[i]>>t) & 0x01];
					  }
				  }
			  }
			  out += outer_header_len*d_fft_len;

			  // Copy data symbols
			  std::vector<int> curr_occupied_carriers;
			  gr_complex* in;
			  //long n_ofdm_symbols = 0; // Number of output items
			  for(int i=0;i<d_device_buffer.size();i++){
				  //int curr_set = 0;
				  in  = d_in_buffer[i];
				  curr_occupied_carriers=d_carriers_buffer[i];
				  int symbols_to_allocate = curr_occupied_carriers.size();
				  int symbols_allocated = 0;
				  int n_curr_ofdm_symbols=0;
				  for (int j = 0; j < d_size[i]; j++) {
					if (symbols_allocated == 0) {
						n_curr_ofdm_symbols++;
					}
					out[(n_curr_ofdm_symbols-1) * d_fft_len + curr_occupied_carriers[symbols_allocated]] = in[j];
					symbols_allocated++;
					if (symbols_allocated == symbols_to_allocate) {
					  symbols_allocated = 0;
					}
				  }
				  //n_ofdm_symbols = n_curr_ofdm_symbols;
				  free(in);
			  }

			  int n_ofdm_symbols = 30;
			  int curr_set = 0;
			  for (int i = 0; i < n_ofdm_symbols; i++) {
				for (unsigned k = 0; k < d_pilot_carriers[curr_set].size(); k++) {
				  out[i * d_fft_len + d_pilot_carriers[curr_set][k]] = d_pilot_symbols[curr_set][k];
				}
				curr_set = (curr_set + 1) % d_pilot_carriers.size();
			  }


			  d_device_buffer.clear();
			  d_in_buffer.clear();
			  d_carriers_buffer.clear();
			  d_size.clear();


			  //store current packet
			  if(device_id!=0){
				  d_device_buffer.push_back(device_id);
				  size=sizeof(gr_complex) * ninput_items[0];
				  in_buffer =(gr_complex*)malloc(size);
				  memcpy( in_buffer,  (const gr_complex *) input_items[0], size);
				  d_in_buffer.push_back(in_buffer);
				  d_carriers_buffer.push_back(occupied_carriers);
				  d_size.push_back(ninput_items[0]);
				  //consume_each(size);
			  }

			  return 30 + d_sync_words.size() + outer_header_len;
		  }
      }
    }

  } /* namespace howto */
} /* namespace gr */

