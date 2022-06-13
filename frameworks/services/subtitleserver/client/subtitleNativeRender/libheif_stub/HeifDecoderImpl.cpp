#include "HeifDecoderImpl.h"

HeifDecoder* createHeifDecoder() {
    return new android::HeifDecoderImpl();
}


