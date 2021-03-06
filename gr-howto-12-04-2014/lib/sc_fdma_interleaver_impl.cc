/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fstream"
#include <gnuradio/io_signature.h>
#include "sc_fdma_interleaver_impl.h"

namespace gr {
  namespace howto {

    sc_fdma_interleaver::sptr
    sc_fdma_interleaver::make(
	int lower_fft_len,
	int upper_fft_len,
	bool forward,
	const std::vector<std::vector<gr_complex> > &sync_words,
	const std::string &len_tag_key
    )
    {
      return gnuradio::get_initial_sptr
        (new sc_fdma_interleaver_impl(
	  lower_fft_len,
	  upper_fft_len,
	  forward,
	  sync_words,
	  len_tag_key
    ));
    }

    /*
     * The private constructor
     */
    sc_fdma_interleaver_impl::sc_fdma_interleaver_impl(
	int lower_fft_len,
	int upper_fft_len,
	bool forward,
	const std::vector<std::vector<gr_complex> > &sync_words,
	const std::string &len_tag_key
    ) : tagged_stream_block("sc_fdma_interleaver",
              io_signature::make(1, 1, sizeof(gr_complex) * (forward ? lower_fft_len : upper_fft_len)),
              io_signature::make(1, 1, sizeof(gr_complex) * (forward ? upper_fft_len : lower_fft_len)), len_tag_key),
	d_lower_fft_len(lower_fft_len),
	d_upper_fft_len(upper_fft_len),
	d_forward(forward),
	d_sync_words(sync_words)
    {
      for (unsigned i = 0; i < d_sync_words.size(); i++) {
	if (d_sync_words[i].size() != (unsigned) d_upper_fft_len) {
	  throw std::invalid_argument("sync words must be fft length");
	}
      }
      
      //set_tag_propagation_policy(TPP_DONT);
      set_relative_rate(1.0);
      /*if(d_forward){
        set_relative_rate(d_upper_fft_len);
      }else{
	set_relative_rate(d_lower_fft_len);
      }*/
    }

    /*
     * Our virtual destructor.
     */
    sc_fdma_interleaver_impl::~sc_fdma_interleaver_impl()
    {
    }

    int
    sc_fdma_interleaver_impl::calculate_output_stream_length(const gr_vector_int &ninput_items)
    {
      int nout = ninput_items[0];
      return nout  + d_sync_words.size();
    }

    int
    sc_fdma_interleaver_impl::work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

	noutput_items = ninput_items[0];

        int factor = d_upper_fft_len / d_lower_fft_len;
	if(d_forward){
		// Copy Sync word
		for (unsigned i = 0; i < d_sync_words.size(); i++) {
  	  	  memcpy((void *) out, (void *) &d_sync_words[i][0], sizeof(gr_complex) * d_upper_fft_len);
	 	   out += d_upper_fft_len;
        	}


		memset((void *) out, 0x00, sizeof(gr_complex) * d_upper_fft_len * noutput_items);
		for(int i = 0;i<noutput_items;i++){
		  for(int j = 0;j<d_lower_fft_len;j++){
			out[factor*j+d_upper_fft_len*i] = in[j+d_lower_fft_len*i];
		  }
		}

		noutput_items += d_sync_words.size();
	}else{
		memset((void *) out, 0x00, sizeof(gr_complex) * d_lower_fft_len * noutput_items);
		for(int i = 0;i<noutput_items;i++){
		  for(int j = 0;j<d_lower_fft_len;j++){
			out[j+d_lower_fft_len*i] = in[factor*j+d_upper_fft_len*i];
		  }
		}
	}
	

        // Tell runtime system how many output items we produced.
        return noutput_items ;
    }

  } /* namespace howto */
} /* namespace gr */

