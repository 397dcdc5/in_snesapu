# in_snesapu

in_snesapu.dll is a plug-in that allows you to use degrade-factory improved SNESAPU.DLL (for other players) with XMPlay.  

![Sample screenshot: in_snesapudlg.](https://github.com/397dcdc5/in_snesapu/in_snesapudlg.png)  

﻿﻿in_snesapu.dll (x86)  
  
Usage: ------------------------  
in_snesapu.dll is a plugin that allows you to use degraded-factory's improved SNESAPU.DLL (for other players) with XMPlay. You need to download the improved SNESAPU.DLL (for other players) from https://dgrfactory.jp/spcplay/ and place it in the same folder as in_snesapu.dll.  
  
  
Note: -------------------------  
We recommend SNESAPU.DLL version 2.20.0 or higher. Even if the recommended version or higher is used, it may not work due to DLL specification changes, etc.  
  
We have not confirmed that   in_snesapu.dll works with XMPlay file version 3.8.5.0 or lower. Also, it may not work with future changes in XMPlay's specifications.  
  
Please check for viruses before using in_snesapu.dll.  
  
  
Disclaimer: -------------------  
No Warranty. Use at your own risk.  
  
  
Other: ------------------------  
XMPlay's PCM data is received as float (IEEE 754), and the improved SNESAPU.DLL seems to calculate waveforms internally using float, so the sample bit is fixed at 32bit. The sample rate is also fixed at 32000 Hz because the actual device is 32000 Hz and the explanation of SetAPUOpt() suggests that the reproducibility may drop. The mixing is also set to the highest quality float. For sample conversion, please use XMPlay's resampler or noise shaping which allows you to specify dithering separately.  
  
Message/Comments/Tags are displayed.
  
Script700 is not supported.  
  
The size of the DLL is large because it is built with /MT.  
  
All this DLL does is transcribe 32bit float PCM data received from SNESAPU.DLL to a float buffer prepared by XMPlay using movsd.  
