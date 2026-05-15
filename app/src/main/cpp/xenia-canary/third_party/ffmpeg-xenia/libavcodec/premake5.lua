project("libavcodec")
  uuid("9DB2830C-D326-48ED-B4CC-08EA6A1B7272")
  kind("StaticLib")
  language("C")
  ffmpeg_common()

  filter("files:not ../../FFmpeg/libavcodec/wmaprodec.c")
    warnings "Off"
  filter({})

  links({
    "libavutil",
  })

  -- Needed for files in subdirectories (e.g. bsf/null.c) that include
  -- headers relative to libavcodec/.
  includedirs({
    "../../FFmpeg/libavcodec",
  })

  -- libavcodec/Makefile:
  --   HEADERS:
  files({
    "../../FFmpeg/libavcodec/ac3_parser.h",
    "../../FFmpeg/libavcodec/adts_parser.h",
    "../../FFmpeg/libavcodec/avcodec.h",
    "../../FFmpeg/libavcodec/avdct.h",
    "../../FFmpeg/libavcodec/bsf.h",
    "../../FFmpeg/libavcodec/codec.h",
    "../../FFmpeg/libavcodec/codec_desc.h",
    "../../FFmpeg/libavcodec/codec_id.h",
    "../../FFmpeg/libavcodec/codec_par.h",
    "../../FFmpeg/libavcodec/d3d11va.h",
    "../../FFmpeg/libavcodec/dirac.h",
    "../../FFmpeg/libavcodec/dv_profile.h",
    "../../FFmpeg/libavcodec/dxva2.h",
    "../../FFmpeg/libavcodec/jni.h",
    "../../FFmpeg/libavcodec/mediacodec.h",
    "../../FFmpeg/libavcodec/packet.h",
    "../../FFmpeg/libavcodec/qsv.h",
    "../../FFmpeg/libavcodec/vdpau.h",
    "../../FFmpeg/libavcodec/version.h",
    "../../FFmpeg/libavcodec/videotoolbox.h",
    "../../FFmpeg/libavcodec/vorbis_parser.h",
  })
  --   OBJS:
  files({
    "../../FFmpeg/libavcodec/ac3_parser.c",
    "../../FFmpeg/libavcodec/adts_parser.c",
    "../../FFmpeg/libavcodec/allcodecs.c",
    "../../FFmpeg/libavcodec/avcodec.c",
    "../../FFmpeg/libavcodec/avdct.c",
    "../../FFmpeg/libavcodec/packet.c",
    "../../FFmpeg/libavcodec/bitstream.c",
    "../../FFmpeg/libavcodec/bitstream_filters.c",
    "../../FFmpeg/libavcodec/bsf.c",
    "../../FFmpeg/libavcodec/codec_desc.c",
    "../../FFmpeg/libavcodec/codec_par.c",
    "../../FFmpeg/libavcodec/d3d11va.c",
    "../../FFmpeg/libavcodec/decode.c",
    "../../FFmpeg/libavcodec/dirac.c",
    "../../FFmpeg/libavcodec/dv_profile.c",
    "../../FFmpeg/libavcodec/encode.c",
    "../../FFmpeg/libavcodec/imgconvert.c",
    "../../FFmpeg/libavcodec/jni.c",
    "../../FFmpeg/libavcodec/mathtables.c",
    "../../FFmpeg/libavcodec/mediacodec.c",
    "../../FFmpeg/libavcodec/mpeg12framerate.c",
    "../../FFmpeg/libavcodec/options.c",
    "../../FFmpeg/libavcodec/parser.c",
    "../../FFmpeg/libavcodec/parsers.c",
    "../../FFmpeg/libavcodec/profiles.c",
    "../../FFmpeg/libavcodec/qsv_api.c",
    "../../FFmpeg/libavcodec/raw.c",
    "../../FFmpeg/libavcodec/utils.c",
    "../../FFmpeg/libavcodec/vorbis_parser.c",
    "../../FFmpeg/libavcodec/xiph.c",
    "../../FFmpeg/libavcodec/dct32_fixed.c",
    "../../FFmpeg/libavcodec/dct32_float.c",
    "../../FFmpeg/libavcodec/faandct.c",
    "../../FFmpeg/libavcodec/faanidct.c",
    "../../FFmpeg/libavcodec/fdctdsp.c",
    "../../FFmpeg/libavcodec/jfdctfst.c",
    "../../FFmpeg/libavcodec/jfdctint.c",
    "../../FFmpeg/libavcodec/idctdsp.c",
    "../../FFmpeg/libavcodec/simple_idct.c",
    "../../FFmpeg/libavcodec/jrevdct.c",
    "../../FFmpeg/libavcodec/mpegaudio.c",
    "../../FFmpeg/libavcodec/mpegaudiodec_common.c",
    "../../FFmpeg/libavcodec/mpegaudiodsp.c",
    "../../FFmpeg/libavcodec/mpegaudiodsp_data.c",
    "../../FFmpeg/libavcodec/mpegaudiodsp_fixed.c",
    "../../FFmpeg/libavcodec/mpegaudiodsp_float.c",
    "../../FFmpeg/libavcodec/mpegaudiodecheader.c",
    "../../FFmpeg/libavcodec/mpegaudiodata.c",
    "../../FFmpeg/libavcodec/sinewin.c",
    "../../FFmpeg/libavcodec/wma_freqs.c",
    "../../FFmpeg/libavcodec/mpegaudiodec_fixed.c",
    "../../FFmpeg/libavcodec/mpegaudiodec_float.c",
    "../../FFmpeg/libavcodec/wmaprodec.c",
    "../../FFmpeg/libavcodec/wma.c",
    "../../FFmpeg/libavcodec/wma_common.c",
    "../../FFmpeg/libavcodec/wmadec.c",
    "../../FFmpeg/libavcodec/aactab.c",
    "../../FFmpeg/libavcodec/mpegaudio_parser.c",
    "../../FFmpeg/libavcodec/pthread.c",
    "../../FFmpeg/libavcodec/pthread_slice.c",
    "../../FFmpeg/libavcodec/pthread_frame.c",
    "../../FFmpeg/libavcodec/get_buffer.c",
    "../../FFmpeg/libavcodec/vlc.c",
    "../../FFmpeg/libavcodec/threadprogress.c",
    "../../FFmpeg/libavcodec/bsf/null.c",
    "../../FFmpeg/libavcodec/exif.c",
    "../../FFmpeg/libavcodec/mpegaudiotabs.c",
    "../../FFmpeg/libavcodec/tiff_common.c",
  })
  filter({"platforms:Windows"})
  files({
    "../../FFmpeg/libavcodec/file_open.c",
  })
  filter({})

  -- libavcodec/aarch64/Makefile:
  --   OBJS:
  filter({"platforms:Android-ARM64"})
  files({
    "../../FFmpeg/libavcodec/aarch64/idctdsp_init_aarch64.c",
    "../../FFmpeg/libavcodec/aarch64/mpegaudiodsp_init.c",
  })
  filter({})
  --   NEON-OBJS:
  filter({"platforms:Android-ARM64"})
  files({
    "../../FFmpeg/libavcodec/aarch64/fft_neon.S",
    "../../FFmpeg/libavcodec/aarch64/simple_idct_neon.S",
    "../../FFmpeg/libavcodec/aarch64/mdct_neon.S",
    "../../FFmpeg/libavcodec/aarch64/mpegaudiodsp_neon.S",
  })
  filter({})

  -- libavcodec/x86/Makefile:
  --   OBJS:
  filter({"platforms:Android-x86_64 or platforms:Linux or platforms:Windows"})
  files({
    "../../FFmpeg/libavcodec/x86/constants.c",
    "../../FFmpeg/libavcodec/x86/fdctdsp_init.c",
    "../../FFmpeg/libavcodec/x86/idctdsp_init.c",
    "../../FFmpeg/libavcodec/x86/mpegaudiodsp.c",
  })
  filter({})
  --   MMX-OBJS:
  filter({"platforms:Android-x86_64 or platforms:Linux or platforms:Windows"})
  files({
    "../../FFmpeg/libavcodec/x86/fdct.c",
  })
  filter({})
