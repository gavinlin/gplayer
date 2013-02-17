#include "config.h"
#include "libswscale/swscale.h"
#include "libswscale/swscale_internal.h"
#include "libavutil/cpu.h"
#include "libswscale/arm/yuv2rgb.h"


#ifdef HAVE_ARMV5TE
static int yuv420_rgb16_ARMV5TE(SwsContext *c, const uint8_t *src[], int srcStride[],
                           int sliceY, int sliceH, uint8_t *dst[], int dstStride[])
{
	yuv420_2_rgb565(dst[0],
			 src[0],
			 src[1],
			 src[2],
			 c->srcW,
			 c->srcH,
			 srcStride[0],
			 srcStride[1],
			 (c->srcW)<<1,
			 yuv2rgb565_table,
			 0);
	return sliceH;
}


av_cold SwsFunc ff_yuv2rgb_init_armv5te(SwsContext *c)
{

	if(c->srcFormat != PIX_FMT_YUV420P)
		return NULL;
		
	switch (c->dstFormat) {
		case PIX_FMT_RGB565: 
			return yuv420_rgb16_ARMV5TE;
		default: return NULL;
        }
}
#endif
