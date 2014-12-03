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

#ifndef INCLUDED_HOWTO_SC_FDMA_CARRIER_ALLOCATOR_CVC_IMPL_H
#define INCLUDED_HOWTO_SC_FDMA_CARRIER_ALLOCATOR_CVC_IMPL_H

#include <howto/sc_fdma_carrier_allocator_cvc.h>

namespace gr {
  namespace howto {

    class sc_fdma_carrier_allocator_cvc_impl : public sc_fdma_carrier_allocator_cvc
    {
     private:
      //! FFT length
      const int d_fft_len;
      //! Which carriers/symbols carry data
      std::vector<std::vector<int> > d_occupied_carriers;
      int d_symbols_per_set;
      const bool d_output_is_shifted;

     protected:
      int calculate_output_stream_length(const gr_vector_int &ninput_items);

     public:
      sc_fdma_carrier_allocator_cvc_impl(
	  int fft_len,
	  const std::vector<std::vector<int> > &occupied_carriers,
	  const std::string &len_tag_key,
	  const bool output_is_shifted
      );
      ~sc_fdma_carrier_allocator_cvc_impl();

      std::string len_tag_key() { return d_length_tag_key_str; };

      const int fft_len() { return d_fft_len; };
      std::vector<std::vector<int> > occupied_carriers() { return d_occupied_carriers; };

      int work(int noutput_items,
		 gr_vector_int &ninput_items,
		 gr_vector_const_void_star &input_items,
		 gr_vector_void_star &output_items);
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_sc_fdma_CARRIER_ALLOCATOR_CVC_IMPL_H */

