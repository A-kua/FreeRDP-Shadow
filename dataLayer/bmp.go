package dataLayer

import (
	"FuckRDP/common"
	"image"
	"image/color"
	"image/draw"
	"sync"
)

type Bitmap struct {
	Pixels            *image.RGBA
	DestLeft, DestTop int
}

var bmpMux sync.Mutex
var bitmapChan = make(chan *Bitmap, 30)
var desktopBitmap = image.NewRGBA(image.Rect(0, 0, 1920, 1080))

func GetBitmap() *image.RGBA {
	bmpMux.Lock()
	defer bmpMux.Unlock()
	return common.CopyRGBA(desktopBitmap)
}

func mergeBmp(bitmap *Bitmap) {
	var (
		drawX = 0
		drawY = 0
	)

	drawX = bitmap.DestLeft
	drawY = bitmap.DestTop
	targetRect := image.Rect(drawX, drawY, drawX+bitmap.Pixels.Bounds().Dx(), drawY+bitmap.Pixels.Bounds().Dy())

	bmpMux.Lock()
	draw.Draw(desktopBitmap, targetRect, bitmap.Pixels, bitmap.Pixels.Bounds().Min, draw.Over)
	bmpMux.Unlock()
}

func BitmapInput(originImg []byte, left, top, width, height, bitsPerPixel int) {
	var (
		i = 0
		r uint8
		g uint8
		b uint8
		a uint8
	)
	var (
		bitIndex          = 0
		byteIndex         = 0
		magicCode1 uint8  = 0
		magicCode2 uint8  = 0
		dataSize1  uint8  = 0
		dataSize2  uint8  = 0
		bakType1   uint8  = 0
		bakType2   uint8  = 0
		bakType3   uint8  = 0
		bakType4   uint8  = 0
		hasData           = true
		data       []byte = nil
	)

	pixels := image.NewRGBA(image.Rect(0, 0, width, height))

	extractFunc := func(b, r uint8) {
		bHigh5 := (b >> 3) & 0x1F
		rHigh5 := (r >> 3) & 0x1F

		bitB := int(bHigh5) % 2
		bitR := int(rHigh5) % 2

		switch byteIndex {
		case 0:
			magicCode1 |= byte(bitB) << (7 - bitIndex)
			magicCode1 |= byte(bitR) << (7 - (bitIndex + 1))
		case 1:
			magicCode2 |= byte(bitB) << (7 - bitIndex)
			magicCode2 |= byte(bitR) << (7 - (bitIndex + 1))
		case 2:
			if magicCode1 != 0xde || magicCode2 != 0xad {
				hasData = false
				return
			}
			dataSize1 |= byte(bitB) << (7 - bitIndex)
			dataSize1 |= byte(bitR) << (7 - (bitIndex + 1))
		case 3:
			dataSize2 |= byte(bitB) << (7 - bitIndex)
			dataSize2 |= byte(bitR) << (7 - (bitIndex + 1))
		case 4:
			bakType1 |= byte(bitB) << (7 - bitIndex)
			bakType1 |= byte(bitR) << (7 - (bitIndex + 1))
		case 5:
			bakType2 |= byte(bitB) << (7 - bitIndex)
			bakType2 |= byte(bitR) << (7 - (bitIndex + 1))
		case 6:
			bakType3 |= byte(bitB) << (7 - bitIndex)
			bakType3 |= byte(bitR) << (7 - (bitIndex + 1))
		case 7:
			bakType4 |= byte(bitB) << (7 - bitIndex)
			bakType4 |= byte(bitR) << (7 - (bitIndex + 1))
		case 8:
			if data == nil {
				data = make([]byte, int(dataSize1)<<8+int(dataSize2)+4) // 4 for CRC32
			}
			fallthrough
		default:
			data[byteIndex-8] |= byte(bitB) << (7 - bitIndex)
			data[byteIndex-8] |= byte(bitR) << (7 - (bitIndex + 1))
		}
		bitIndex += 2
		if bitIndex >= 8 {
			bitIndex = 0
			byteIndex++
		}
		if hasData && data != nil && byteIndex-8 >= cap(data) {
			hasData = false
		}
	}
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			r, g, b, a = common.ToRGBA(bitsPerPixel, i, originImg)
			pixels.SetRGBA(x, y, color.RGBA{R: r, G: g, B: b, A: a})
			i += bitsPerPixel
			if hasData {
				extractFunc(b, r)
			}
		}
	}
	bitmapChan <- &Bitmap{
		Pixels:   pixels,
		DestLeft: left,
		DestTop:  top,
	}
	if data != nil {
		dataQueue <- embedData{
			data:    data,
			bakType: uint32(bakType1)<<24 | uint32(bakType2)<<16 | uint32(bakType3)<<8 | uint32(bakType4),
		}
	}
}
