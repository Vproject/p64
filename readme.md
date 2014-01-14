PVRG-p64 with 4CIF support
==========================
Features
--------
- encode 4CIF streams (yuv4mpeg input only)
- decode 4CIF streams (yuv4mpeg output only)
- decode single CIF streams that incorrectly claim 4CIF as type in their picture header (pvrg-p64, ffmpeg/libav)

Usage notes
-----------
Encoding example
```bash
$ ./p64 -CIF -b 299 -r 512000 -s output.704x576-4x512k.p64 -y4m -z .y4m input.704x576
```
will encode 300 frames of ```input.704x576.y4m``` to 300x4 CIF ```output.704x576-4x512k.p64``` in 4CIF mode (detected based on dimensions stored in ```input.704x576.y4m```), using 512kbit/s as target bitrate for all 1200 CIF frames.

Issues
------
- hacky
- low quality
-- better off scaling up a decoded single 512kbit/s CIF stream to 704x576 than using a 4x512kbit/s 4CIF stream
- does not support switching resolutions mid stream, not even CIF <-> 4CIF
- (not related to 4CIF) might not always decode streams encoded with libavcodec (ffmpeg / libav)
