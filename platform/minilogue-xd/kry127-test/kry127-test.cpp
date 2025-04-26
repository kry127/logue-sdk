/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 * File: waves.cpp
 *
 * Morphing wavetable oscillator
 *
 */
#include <cstdlib>
#include "userosc.h"
#include "fx_api.h"
#include "kry127-test.hpp"

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
}

static q31_t wraparound = 0;

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  
  for (; y != y_e; ) {
    
    *(y++) = wraparound;

    const int random_value = std::rand();
    if (random_value & 2 == 0) {
      wraparound+=10000000;
    } else {
      wraparound-=10000000;
    }
  }
  
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
  (void)params;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  
  switch (index) {
  case k_user_osc_param_id1:
    break;
    
  case k_user_osc_param_id2:
    break;
    
  case k_user_osc_param_id3:
    break;
    
  case k_user_osc_param_id4:
    break;
    
  case k_user_osc_param_id5:
    break;
    
  case k_user_osc_param_id6:
    break;
    
  case k_user_osc_param_shape:
    break;
    
  case k_user_osc_param_shiftshape:
    break;
    
  default:
    break;
  }
}

