PVRG-p64 with 4CIF support
==========================
Features
--------
- encode 4CIF streams (yuv4mpeg input only)
- decode 4CIF streams (yuv4mpeg output only)
- decode single CIF streams that incorrectly claim 4CIF as type in their picture header (pvrg-p64, ffmpeg/libav)

Issues
------
- hacky
- low quality
-- better off scaling up a decoded single 512kbit/s CIF stream to 704x576 than using a 4x512kbit/s 4CIF stream
- does not support switching resolutions mid stream, not even CIF <-> 4CIF
- (not related to 4CIF) might not always decode streams encoded with libavcodec (ffmpeg / libav)
