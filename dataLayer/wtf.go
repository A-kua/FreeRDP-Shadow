package dataLayer

import "FuckRDP/grdp/glog"

func init() {
	RegisterHandler(3739147998, func(bytes []byte) {
		for i, d := range bytes {
			if d != 222 && d != 252 && d != 248 {
				glog.Infof("wtf?? in %d is %d.", i, d)
			}
		}
	})
}

// 151 152 x 154 155 x 157 158 x 160
