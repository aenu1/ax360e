project("libavformat")
  uuid("CEF2E128-AA08-4A36-8045-0AA68A556364")
  kind("StaticLib")
  language("C")
  ffmpeg_common()

  filter("files:not wmaprodec.c")
    warnings "Off"
  filter({})

  links({
    "libavutil",
    "libavcodec",
  })

  -- libavformat/Makefile:
  --   HEADERS:
  files({
    "../../FFmpeg/libavformat/avformat.h",
    "../../FFmpeg/libavformat/avio.h",
    "../../FFmpeg/libavformat/version.h",
  })
  --   OBJS:
  files({
    "../../FFmpeg/libavformat/allformats.c",
    "../../FFmpeg/libavformat/avio.c",
    "../../FFmpeg/libavformat/aviobuf.c",
    "../../FFmpeg/libavformat/dump.c",
    "../../FFmpeg/libavformat/format.c",
    "../../FFmpeg/libavformat/id3v1.c",
    "../../FFmpeg/libavformat/id3v2.c",
    "../../FFmpeg/libavformat/metadata.c",
    "../../FFmpeg/libavformat/mux.c",
    "../../FFmpeg/libavformat/options.c",
    "../../FFmpeg/libavformat/os_support.c",
    "../../FFmpeg/libavformat/protocols.c",
    "../../FFmpeg/libavformat/riff.c",
    "../../FFmpeg/libavformat/sdp.c",
    "../../FFmpeg/libavformat/url.c",
    "../../FFmpeg/libavformat/utils.c",
    "../../FFmpeg/libavformat/riffdec.c",
    "../../FFmpeg/libavformat/asfdec_f.c",
    "../../FFmpeg/libavformat/asf.c",
    "../../FFmpeg/libavformat/asfcrypt.c",
    "../../FFmpeg/libavformat/avlanguage.c",
    "../../FFmpeg/libavformat/mp3dec.c",
    "../../FFmpeg/libavformat/replaygain.c",
    "../../FFmpeg/libavformat/file.c",
    "../../FFmpeg/libavformat/demux.c",
    "../../FFmpeg/libavformat/avformat.c",
    "../../FFmpeg/libavformat/seek.c",
    "../../FFmpeg/libavformat/demux_utils.c",
    "../../FFmpeg/libavformat/urldecode.c",
    "../../FFmpeg/libavformat/asf_tags.c",
    "../../FFmpeg/libavformat/to_upper4.c",
  })
  filter({"platforms:Windows"})
  files({
    "../../FFmpeg/libavformat/file_open.c",
  })
  filter({})
