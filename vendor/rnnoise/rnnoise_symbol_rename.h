#ifndef RNNOISE_SYMBOL_RENAME_H
#define RNNOISE_SYMBOL_RENAME_H
/* Auto-applied via force-include for the vendored rnnoise static lib only.
   RNNoise vendors CELT/Opus-derived code whose symbols collide with libopus
   when both are statically linked. Prefix all internal symbols (the public
   rnnoise_* API is intentionally left untouched). */
#define _celt_autocorr rnnoiseint__celt_autocorr
#define _celt_lpc rnnoiseint__celt_lpc
#define celt_fir rnnoiseint_celt_fir
#define celt_iir rnnoiseint_celt_iir
#define celt_pitch_xcorr rnnoiseint_celt_pitch_xcorr
#define compute_band_corr rnnoiseint_compute_band_corr
#define compute_band_energy rnnoiseint_compute_band_energy
#define compute_dense rnnoiseint_compute_dense
#define compute_gru rnnoiseint_compute_gru
#define compute_rnn rnnoiseint_compute_rnn
#define interp_band_gain rnnoiseint_interp_band_gain
#define opus_fft_alloc rnnoiseint_opus_fft_alloc
#define opus_fft_alloc_arch_c rnnoiseint_opus_fft_alloc_arch_c
#define opus_fft_alloc_twiddles rnnoiseint_opus_fft_alloc_twiddles
#define opus_fft_c rnnoiseint_opus_fft_c
#define opus_fft_free rnnoiseint_opus_fft_free
#define opus_fft_free_arch_c rnnoiseint_opus_fft_free_arch_c
#define opus_fft_impl rnnoiseint_opus_fft_impl
#define opus_ifft_c rnnoiseint_opus_ifft_c
#define pitch_downsample rnnoiseint_pitch_downsample
#define pitch_filter rnnoiseint_pitch_filter
#define pitch_search rnnoiseint_pitch_search
#define remove_doubling rnnoiseint_remove_doubling
#define common rnnoiseint_common
#endif
