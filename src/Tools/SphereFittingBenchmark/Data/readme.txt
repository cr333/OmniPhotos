hotel_0_centre_depth.png is a depth map.
hotel_0_centre_rgb.jpeg is the corresponding color image.
I storage the depth data in 3 channel, instead of uint_16 data type.
the real depth of each pixel compute by  
depth = ( r + g * 256 + b * 256 * 256) / 65535.0