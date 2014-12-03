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

#ifndef INCLUDED_HOWTO_OFDM_CARRIER_ALLOCATOR_CVC_IMPL_H
#define INCLUDED_HOWTO_OFDM_CARRIER_ALLOCATOR_CVC_IMPL_H

#include <howto/ofdm_carrier_allocator_cvc.h>
#include <howto/constellation.h>

namespace gr {
  namespace howto {

    //info of every subcarrier
    typedef struct
    {
        float value;
        float NCR;
        int index;
        int available;
    } power;

    //user info
    typedef struct
    {
        int index;
        int allocated;
    } user;

    class ofdm_carrier_allocator_cvc_impl : public ofdm_carrier_allocator_cvc
    {
     private:
      int d_num_user;
      float d_total_power;
      int d_bandwidth;
      float d_noise_density;
      //! FFT length
      const int d_fft_len;
      //! Which carriers/symbols carry data
      std::vector<std::vector<int> > d_occupied_carriers;
      //! Which carriers/symbols carry pilots symbols
      std::vector<std::vector<int> > d_pilot_carriers;
      //! Value of said pilot symbols
      const std::vector<std::vector<gr_complex> > d_pilot_symbols;
      //! Synch words
      const std::vector<std::vector<gr_complex> > d_sync_words;
      int d_symbols_per_set;
      const bool d_output_is_shifted;
      int d_state;

      std::vector<int> d_temp1; // buffer to allocated channel
      std::vector<int> d_temp2; // buffer to allocated user
      
      std::vector<gr_complex*>  d_in_buffer; //!< Temporary buffer for input
      std::vector<unsigned>  d_device_buffer;
      std::vector<std::vector<int> > d_carriers_buffer;
      std::vector<int> d_size;

      gr::howto::constellation_sptr  d_constellation;
      std::vector<gr_complex> d_symbol_table;


      void reset(std::vector<power> *ptr_init_power_allo, std::vector<user> *ptr_usr);
	  bool check_existence(unsigned device_id);

     protected:
      int calculate_output_stream_length(const gr_vector_int &ninput_items);

     public:
      ofdm_carrier_allocator_cvc_impl(
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
      );
      ~ofdm_carrier_allocator_cvc_impl();

      std::string len_tag_key() { return d_length_tag_key_str; };

      const int fft_len() { return d_fft_len; };
      std::vector<std::vector<int> > occupied_carriers() { return d_occupied_carriers; };
      std::vector<std::vector<int> > pilot_carriers() { return d_pilot_carriers; };

      

      int work(int noutput_items,
		 gr_vector_int &ninput_items,
		 gr_vector_const_void_star &input_items,
		 gr_vector_void_star &output_items);
    };

  } // namespace digital
} // namespace gr

#endif /* INCLUDED_DIGITAL_OFDM_CARRIER_ALLOCATOR_CVC_IMPL_H */

