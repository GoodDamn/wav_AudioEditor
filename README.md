# wav_AudioEditor
Mini audio editor for .wav files

You can change any options of audio files (sample rate (frequency), bitDepth, bit rate and etc.)
Also, you can compress your .wav file and reduce file size and control reducing of "compressSamples"(1 on "compressSamples" means than next "compressSamples" will be deleted of currentSample)

For example, the input file is "Hymn(48000).wav" which contains a lot of samples and has about ~22 Mbytes
This code compresses that file and outputs "programWav.wav" with size ~1.0 Mbytes

In addition, code can create additional samples for "smooth playing". (programWav_uncompressed.wav)

Also, you can add some effects to your output .wav file.
You can:
  1) Change the playback speed (0.5, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25 and +infinity :) (non-negative numbers));
  2) "Repeating" effect (from left to right);
  3) Add fade-in and fade-out effect with 4 interpolators (linear, square, cubic, root) and with custom duration(in seconds);
  4) Change the volume of track (0.0 - 1.0f);
  5) Trim audio file (from "startPosition" to "endPosition" in milliseconds)
 
