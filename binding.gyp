{
  "targets": [
    {
      "target_name": "video_engine",
      "sources": [
        "src/video_engine.cc",
        "src/ffmpeg_decoder.cc",
        "src/opengl_renderer.cc",
        "src/timeline_scheduler.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "src"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "conditions": [
        ["OS=='win'", {
          "defines": [ "WINDOWS" ],
          "libraries": [
            "-lavcodec.lib",
            "-lavformat.lib",
            "-lavutil.lib",
            "-lswscale.lib",
            "-lopengl32.lib"
          ],
          "include_dirs": [
            "C:/ffmpeg/include",
            "C:/opengl/include"
          ],
          "library_dirs": [
            "C:/ffmpeg/lib",
            "C:/opengl/lib"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          },
          "libraries": [
            "-lavcodec",
            "-lavformat",
            "-lavutil",
            "-lswscale",
            "-framework OpenGL"
          ]
        }],
        ["OS=='linux'", {
          "libraries": [
            "-lavcodec",
            "-lavformat",
            "-lavutil",
            "-lswscale",
            "-lGL"
          ],
          "cflags": [
            "`pkg-config --cflags gtk+-3.0`"
          ]
        }]
      ]
    }
  ]
}
