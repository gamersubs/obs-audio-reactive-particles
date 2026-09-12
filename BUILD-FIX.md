The workflow now bootstraps the official OBS plugin template CMake build system, downloads pinned OBS dependencies, and configures libobs before compiling. The requested ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION workaround is retained.


## v9 audio fix

Fixed audio-meter interpretation: `obs_volmeter` callback values are normalized 0..1, not dB. The previous version incorrectly passed them through `obs_db_to_mul`, making normal audio levels almost zero. v9 uses magnitude/peak directly and keeps the Test Mode diagnostic.
