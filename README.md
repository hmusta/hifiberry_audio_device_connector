# hifiberry_audio_device_connector
C code: based on https://github.com/zonque/simple-alsa-loop. Loops hw:2,0 to hw:2,0, then checks for sample rate changes and resets accordingly

bash code: Connects the S/PDIF input of the hifiberry-digi+ I/O to some other output
    Your particular hardware devices can be configured with the exvironment variables at the top. I also have an external USB sound card, so that's assigned to hw:1,0 instead of the hifiberry

Requires the patched version of sound/soc/bcm/hifiberry_digi.c from https://github.com/hmusta/linux (forked from https://github.com/antorsae/linux), since the input sampling rate is queried from /proc/asound/sndrpihifiberry/registers

