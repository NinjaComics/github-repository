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


#ifndef INCLUDED_HOWTO_OFDM_SERIALIZER_VCC_H
#define INCLUDED_HOWTO_OFDM_SERIALIZER_VCC_H

#include <howto/api.h>
#include <gnuradio/tagged_stream_block.h>
#include <howto/ofdm_carrier_allocator_cvc.h>

namespace gr {
  namespace howto {

    /*!
     * \brief Serializes complex modulations symbols from OFDM sub-carriers
     * \ingroup ofdm_blk
     *
     * \details
     * This is the inverse block to the carrier_allocator_cvc. It outputs the
     * complex data symbols as a tagged stream, discarding the pilot symbols.
     *
     * If given, two different tags are parsed: The first key (\p len_tag_key)
     * specifies the number of OFDM symbols in the frame at the input. The
     * second key (\p packet_len_tag_key) specifies the number of complex symbols
     * that are coded into this frame. If given, this second key is then used
     * at the output, otherwise, \p len_tag_key is used.
     * If both are given, the packet length specifies the maximum number of
     * output items, and the frame length specifies the exact number of
     * consumed input items.
     *
     * It is possible to correct a carrier offset in this function by passing
     * another tag with said offset.
     *
     * Input: Complex vectors of length \p fft_len
     * Output: Complex scalars, in the same order as specified in occupied_carriers.
     */
    class HOWTO_API ofdm_serializer_vcc : virtual public tagged_stream_block
    {
     public:
      typedef boost::shared_ptr<ofdm_serializer_vcc> sptr;

      /*!
       * \param fft_len FFT length
       * \param occupied_carriers See ofdm_carrier_allocator_cvc.
       * \param len_tag_key The key of the tag identifying the length of the input frame in OFDM symbols.
       * \param packet_len_tag_key The key of the tag identifying the number of complex symbols in this packet.
       * \param symbols_skipped If the first symbol is not allocated as in \p occupied_carriers[0], set this
       * \param carr_offset_key When this block should correct a carrier offset, specify the tag key of the offset here (not necessary if following an ofdm_frame_equalizer_vcvc)
       * \param input_is_shifted If the input has the DC carrier on index 0 (i.e. it is not FFT shifted), set this to false
       */
      static sptr make(
	  int fft_len,
	  //const std::vector<std::vector<int> > &occupied_carriers,
	  const std::string &len_tag_key="frame_len",
	  const std::string &packet_len_tag_key="",
	  int symbols_skipped=0,
	  const std::string &carr_offset_key="",
	  bool input_is_shifted=true
      );

      /*!
       * \param allocator The carrier allocator block of which this shall be the inverse
       * \param packet_len_tag_key The key of the tag identifying the number of complex symbols in this packet.
       * \param symbols_skipped If the first symbol is not allocated as in \p occupied_carriers[0], set this
       * \param carr_offset_key When this block should correct a carrier offset, specify the tag key of the offset here (not necessary if following an ofdm_frame_equalizer_vcvc)
       * \param input_is_shifted If the input has the DC carrier on index 0 (i.e. it is not FFT shifted), set this to false
       */
      static sptr make(
	  const gr::howto::ofdm_carrier_allocator_cvc::sptr &allocator,
	  const std::string &packet_len_tag_key="",
	  int symbols_skipped=0,
	  const std::string &carr_offset_key="",
	  bool input_is_shifted=true
      );
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_OFDM_SERIALIZER_VCC_H */

