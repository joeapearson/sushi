#include "gtest/gtest.h"

#include "library/vst2x_midi_event_fifo.h"

using namespace sushi;
using namespace sushi::vst2;

namespace {
    constexpr int TEST_FIFO_CAPACITY = 128;
    constexpr int TEST_DATA_SIZE = 100;
} // anonymous namespace

class TestVst2xMidiEventFIFO : public ::testing::Test
{
protected:
    TestVst2xMidiEventFIFO()
    {
    }

    void SetUp()
    {
        // Pre-fill queue
        for (int i=0; i<TEST_DATA_SIZE; i++)
        {
            auto ev = Event::make_note_on_event(0, i, i, 1.0f);
            ASSERT_EQ(true, _module_under_test.push(ev));
        }
    }

    void TearDown()
    {
    }

    Vst2xMidiEventFIFO<TEST_FIFO_CAPACITY> _module_under_test;
};

TEST_F(TestVst2xMidiEventFIFO, test_non_overflowing_behaviour)
{

    auto vst_events = _module_under_test.flush();
    ASSERT_EQ(TEST_DATA_SIZE, vst_events->numEvents);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[i]);
        EXPECT_EQ(i, midi_ev->deltaFrames);
    }
}

TEST_F(TestVst2xMidiEventFIFO, test_flush)
{
    _module_under_test.flush();
    auto vst_events = _module_under_test.flush();
    EXPECT_EQ(0, vst_events->numEvents);
}

TEST_F(TestVst2xMidiEventFIFO, test_overflow)
{
    constexpr int overflow_offset = 1000;

    for (int i=TEST_DATA_SIZE; i<TEST_FIFO_CAPACITY; i++)
    {
        auto ev = Event::make_note_on_event(0, i, 0, 1.0f);
        ASSERT_EQ(true, _module_under_test.push(ev));
    }
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto ev = Event::make_note_on_event(0, overflow_offset+i, 0, 1.0f);
        ASSERT_EQ(false, _module_under_test.push(ev));
    }
    auto vst_events = _module_under_test.flush();
    ASSERT_EQ(TEST_FIFO_CAPACITY, vst_events->numEvents);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[i]);
        EXPECT_EQ(overflow_offset + i, midi_ev->deltaFrames);
    }
}

TEST_F(TestVst2xMidiEventFIFO, test_flush_after_overflow)
{
    // Let the queue overflow...
    for (int i=0; i<(2*TEST_FIFO_CAPACITY); i++)
    {
        auto ev = Event::make_note_on_event(0, i, 0, 1.0f);
        _module_under_test.push(ev);
    }
    _module_under_test.flush();

    // ... and check that after flushing is working again in normal, non-overfowed conditions
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto ev = Event::make_note_on_event(0, i, 0, 1.0f);
        ASSERT_EQ(true, _module_under_test.push(ev));
    }
    auto vst_events = _module_under_test.flush();
    ASSERT_EQ(TEST_DATA_SIZE, vst_events->numEvents);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[i]);
        EXPECT_EQ(i, midi_ev->deltaFrames);
    }
}

