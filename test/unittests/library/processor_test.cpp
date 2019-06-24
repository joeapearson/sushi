#include "gtest/gtest.h"
#include "engine/transport.h"

#define private public
#define protected public

#include "library/processor.h"
#include "test_utils/host_control_mockup.h"

using namespace sushi;

/* Implement dummies of virtual methods to we can instantiate a test class */
class ProcessorTest : public Processor
{
public:
    ProcessorTest(HostControl host_control) : Processor(host_control) {}
    virtual ~ProcessorTest() {}
    virtual void process_audio(const ChunkSampleBuffer& /*in_buffer*/,
                               ChunkSampleBuffer& /*out_buffer*/) override {}
    virtual void process_event(RtEvent /*event*/) override {}
};


class TestProcessor : public ::testing::Test
{
protected:
    TestProcessor()
    {
    }
    void SetUp()
    {
        _module_under_test = new ProcessorTest(_host_control.make_host_control_mockup());
    }
    void TearDown()
    {
        delete(_module_under_test);
    }
    HostControlMockup _host_control;
    Processor* _module_under_test;
};

TEST_F(TestProcessor, TestBasicProperties)
{
    /* Set the common properties and verify the changes are applied */
    _module_under_test->set_name(std::string("Processor 1"));
    EXPECT_EQ(_module_under_test->name(), "Processor 1");

    _module_under_test->set_label("processor_1");
    EXPECT_EQ("processor_1", _module_under_test->label());

    _module_under_test->set_enabled(true);
    EXPECT_TRUE(_module_under_test->enabled());
}

TEST_F(TestProcessor, TestParameterHandling)
{
    /* Register a single parameter and verify accessor functions */
    auto p = new FloatParameterDescriptor("param", "Float", 0, 1, nullptr);
    _module_under_test->register_parameter(p);

    auto param = _module_under_test->parameter_from_name("not_found");
    EXPECT_FALSE(param);
    param = _module_under_test->parameter_from_name("param");
    EXPECT_TRUE(param);

    ObjectId id = param->id();
    param = _module_under_test->parameter_from_id(id);
    EXPECT_TRUE(param);
    param = _module_under_test->parameter_from_id(1000);
    EXPECT_FALSE(param);

    auto param_list = _module_under_test->all_parameters();
    EXPECT_EQ(1u, param_list.size());
}

TEST(TestProcessorUtils, TestSetBypassRampTime)
{
    int chunks_in_10ms = (TEST_SAMPLE_RATE * 0.01) / AUDIO_CHUNK_SIZE;
    EXPECT_EQ(chunks_in_10ms, chunks_to_ramp(TEST_SAMPLE_RATE));
}

class TestBypassManager : public ::testing::Test
{
protected:
    TestBypassManager() {}

    BypassManager _module_under_test{false};
};

TEST_F(TestBypassManager, TestOperation)
{
    EXPECT_FALSE(_module_under_test.bypassed());
    EXPECT_TRUE(_module_under_test.should_process());
    EXPECT_FALSE(_module_under_test.should_ramp());

    // Set the same condition, nothing should change
    _module_under_test.set_bypass(false, TEST_SAMPLE_RATE);
    EXPECT_FALSE(_module_under_test.bypassed());
    EXPECT_TRUE(_module_under_test.should_process());
    EXPECT_FALSE(_module_under_test.should_ramp());

    // Set bypass on
    _module_under_test.set_bypass(true, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.bypassed());
    EXPECT_TRUE(_module_under_test.should_process());
    EXPECT_TRUE(_module_under_test.should_ramp());
}

TEST_F(TestBypassManager, TestRamping)
{
    int chunks_in_ramp = (TEST_SAMPLE_RATE * 0.01) / AUDIO_CHUNK_SIZE;
    ChunkSampleBuffer buffer(2);
    _module_under_test.set_bypass(true, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.should_ramp());

    for (int i = 0; i < chunks_in_ramp ; ++i)
    {
        test_utils::fill_sample_buffer(buffer, 1.0f);
        _module_under_test.ramp(buffer);
    }

    // We should now have ramped down to 0
    EXPECT_FLOAT_EQ(0.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ(0.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ(1.0f / chunks_in_ramp, buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(1.0f / chunks_in_ramp, buffer.channel(1)[0]);

    EXPECT_FALSE(_module_under_test.should_ramp());

    // Turn it on again (bypass = false)
    _module_under_test.set_bypass(false, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.should_ramp());

    for (int i = 0; i < chunks_in_ramp ; ++i)
    {
        test_utils::fill_sample_buffer(buffer, 1.0f);
        _module_under_test.ramp(buffer);
    }

    // We should have ramped up to full volume again
    EXPECT_FLOAT_EQ(1.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ(1.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ((chunks_in_ramp - 1.0f) / chunks_in_ramp, buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ((chunks_in_ramp - 1.0f) / chunks_in_ramp, buffer.channel(1)[0]);

    EXPECT_FALSE(_module_under_test.should_ramp());
}
