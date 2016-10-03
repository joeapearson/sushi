/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef EQUALIZER_PLUGIN_H
#define EQUALIZER_PLUGIN_H

#include "plugin_interface.h"
#include "biquad_filter.h"

namespace equalizer_plugin {

enum equalizer_parameter_id
{
    FREQUENCY = 1,
    GAIN,
    Q
};

class EqualizerPlugin : public AudioProcessorBase
{
public:
    EqualizerPlugin();
    ~EqualizerPlugin();
    AudioProcessorStatus init(const AudioProcessorConfig& configuration) override ;

    void set_parameter(unsigned int parameter_id, float value) override ;

    void process(const float *in_buffer, float *out_buffer) override;

private:
    biquad::BiquadFilter _filter;
    AudioProcessorConfig _configuration;

    float _freq{1000.0f};
    float _gain{0.0f};
    float _q{1.0f};
};

}// namespace equalizer_plugin
#endif // EQUALIZER_PLUGIN_H