# InputAnalyzer-adjusted-Estlack-2021-git

This is an early attempt at an audio reactive graphic animation display. Right now, I am working to set it up so I can read audio inputs from a MacBook Pro's microphone. The InputAnalyzer takes the signal and produces live readings of the frequency and amplitudes of FFT bins, using MonitorSpectralNode. Much of this was originally authored by Richard Eakin, the architect behind Cinder's audio library. 

Building on this initial Sample, I'm looking to pull values from specific Spectral Node Bins (frequency and amplitude), and trigger graphic animations (or other OpenGL, Metal) events based on changes in those live values.
