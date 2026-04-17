#include <emscripten\bind.h>
#include <emscripten\val.h>
#include "samplerate.h"
#include <vector>
#include <exception>
#include <stdexcept>
#include <emscripten/console.h>

enum ConverterType
{
    BestQuality = SRC_SINC_BEST_QUALITY,
    MediumQuality = SRC_SINC_MEDIUM_QUALITY,
    Fastest = SRC_SINC_FASTEST,
    ZeroOrderHold = SRC_ZERO_ORDER_HOLD,
    Linear = SRC_LINEAR,
};

EMSCRIPTEN_DECLARE_VAL_TYPE(Float32Array);
EMSCRIPTEN_DECLARE_VAL_TYPE(ReadCallback);

class SampleRateConverter {
    SRC_STATE* ptr;
    ReadCallback callback;
    float buffer[1024] = {};

public:
    SampleRateConverter(ConverterType type, int channels, ReadCallback callback)
        : callback(std::move(callback))
    {
        int error = 0;
        ptr = src_callback_new(SampleRateConverter::readThunk, type, channels, &error, this);
    }

    void read(double src_ratio, Float32Array buffer) {
        std::vector<float> frames;

        auto bufferLength = buffer["length"].as<unsigned>();
        frames.resize(bufferLength);

        int read = src_callback_read(ptr, src_ratio, bufferLength, frames.data());
        if (read == -1) throw std::runtime_error("Could not read resample data");

        buffer.call<void>("set", emscripten::val(emscripten::typed_memory_view(read, frames.data())));
    }

    ~SampleRateConverter() {
        src_delete(ptr);
    }

private:
    static long readThunk(void* cb_data, float** data) {
        auto self = (SampleRateConverter*)cb_data;
        *data = self->buffer;
        return self->callback(Float32Array(emscripten::val(emscripten::typed_memory_view(1024, self->buffer)))).as<long>();
    }
};

EMSCRIPTEN_BINDINGS(libsamplerate) {
    emscripten::register_type<Float32Array>("Float32Array");
    emscripten::register_type<ReadCallback>("ReadCallback", "{ (array: Float32Array): number }");

    emscripten::enum_<ConverterType>("ConverterType")
        .value("BestQuality", ConverterType::BestQuality)
        .value("MediumQuality", ConverterType::MediumQuality)
        .value("Fastest", ConverterType::Fastest)
        .value("ZeroOrderHold", ConverterType::ZeroOrderHold)
        .value("Linear", ConverterType::Linear)
        ;

    emscripten::class_<SampleRateConverter>("SampleRateConverter")
        .constructor<ConverterType, int, ReadCallback>()
        .function("read", &SampleRateConverter::read)
        ;
}
