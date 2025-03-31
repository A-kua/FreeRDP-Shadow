package common

import (
	"FuckRDP/grdp/core"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/pdu"
	"image"
	"image/draw"
)

// CopyRGBA 函数用于拷贝 *image.RGBA 对象
func CopyRGBA(src *image.RGBA) *image.RGBA {
	// 创建一个新的 image.RGBA 对象，大小与原对象相同
	dst := image.NewRGBA(src.Bounds())
	// 使用 draw.Draw 函数将原对象的像素数据复制到新对象中
	draw.Draw(dst, dst.Bounds(), src, src.Bounds().Min, draw.Src)
	return dst
}

func BitmapDecompress(bitmap *pdu.BitmapData) []byte {
	return core.Decompress(bitmap.BitmapDataStream, int(bitmap.Width), int(bitmap.Height), Bpp(bitmap.BitsPerPixel))
}

// Bpp 计算每像素所需的字节数
func Bpp(BitsPerPixel uint16) (pixel int) {
	switch BitsPerPixel {
	case 15:
		pixel = 1
	case 16:
		pixel = 2
	case 24:
		pixel = 3
	case 32:
		pixel = 4
	default:
		glog.Error("非法位图数据")
	}
	return
}

func ToRGBA(bpp int, i int, data []byte) (r, g, b, a uint8) {
	a = 255
	switch bpp {
	case 1:
		rgb555 := core.Uint16BE(data[i], data[i+1])
		r, g, b = core.RGB555ToRGB(rgb555)
	case 2:
		rgb565 := core.Uint16BE(data[i], data[i+1])
		r, g, b = core.RGB565ToRGB(rgb565)
		//fmt.Println("hu: ", rgb565)
	case 3, 4:
		fallthrough
	default:
		r, g, b = data[i+2], data[i+1], data[i]
	}

	return
}
