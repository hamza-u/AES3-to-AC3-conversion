AES3-to-AC3-conversion
======================

This app converts aes3 embedded ac3 audio to AC3 audio.

```Usage: ./aes3_to_ac3.out <input.aes3/pcm/raw> <output.ac3> <num pkts to dump>```

This app converts aes3 to AC3.
"num pkts to dump" parameter can be set to very high value to process entire file.

Compilation
============
gcc -o aes3_to_ac3.out extract_ac3_from_aes3.c
