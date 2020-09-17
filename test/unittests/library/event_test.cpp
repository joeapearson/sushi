#include "gtest/gtest.h"

#include "library/event.cpp"

#undef private

#include "engine/audio_engine.h"

#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"

using namespace midi;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::control_frontend;
using namespace sushi::midi_dispatcher;

static int dummy_processor_callback(void* /*arg*/, EventId /*id*/)
{
    return 0;
}

constexpr float TEST_SAMPLE_RATE = 44100;

TEST(EventTest, TestToRtEvent)
{
    auto note_on_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, 1, 0, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_on_event.is_keyboard_event());
    EXPECT_TRUE(note_on_event.maps_to_rt_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, note_on_event.time());
    RtEvent rt_event = note_on_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_ON, rt_event.type());
    EXPECT_EQ(5, rt_event.sample_offset());
    EXPECT_EQ(1u, rt_event.keyboard_event()->processor_id());
    EXPECT_EQ(48, rt_event.keyboard_event()->note());
    EXPECT_EQ(0, rt_event.keyboard_event()->channel());
    EXPECT_FLOAT_EQ(1.0f, rt_event.keyboard_event()->velocity());

    auto note_off_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, 1, 1, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_off_event.is_keyboard_event());
    EXPECT_TRUE(note_off_event.maps_to_rt_event());
    rt_event = note_off_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_OFF, rt_event.type());

    auto note_at_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, 1, 2, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_at_event.is_keyboard_event());
    EXPECT_TRUE(note_at_event.maps_to_rt_event());
    rt_event = note_at_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_AFTERTOUCH, rt_event.type());;

    auto pitchbend_event = KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, 2, 3, 0.5f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(pitchbend_event.is_keyboard_event());
    rt_event = pitchbend_event.to_rt_event(6);
    EXPECT_EQ(RtEventType::PITCH_BEND, rt_event.type());
    EXPECT_EQ(6, rt_event.sample_offset());
    EXPECT_EQ(2u, rt_event.keyboard_common_event()->processor_id());
    EXPECT_FLOAT_EQ(0.5f, rt_event.keyboard_common_event()->value());

    auto modulation_event = KeyboardEvent(KeyboardEvent::Subtype::MODULATION, 3, 4, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(modulation_event.is_keyboard_event());
    EXPECT_TRUE(modulation_event.maps_to_rt_event());
    rt_event = modulation_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::MODULATION, rt_event.type());

    auto aftertouch_event = KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, 4, 5, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(aftertouch_event.is_keyboard_event());
    EXPECT_TRUE(aftertouch_event.maps_to_rt_event());
    rt_event = aftertouch_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::AFTERTOUCH, rt_event.type());

    auto midi_event = KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI, 5, {1,2,3,4}, IMMEDIATE_PROCESS);
    EXPECT_TRUE(midi_event.is_keyboard_event());
    rt_event = midi_event.to_rt_event(7);
    EXPECT_EQ(RtEventType::WRAPPED_MIDI_EVENT, rt_event.type());
    EXPECT_EQ(7, rt_event.sample_offset());
    EXPECT_EQ(5u, rt_event.wrapped_midi_event()->processor_id());
    EXPECT_EQ(MidiDataByte({1,2,3,4}), rt_event.wrapped_midi_event()->midi_data());

    auto param_ch_event = ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE, 6, 50, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(param_ch_event.is_parameter_change_event());
    EXPECT_TRUE(param_ch_event.maps_to_rt_event());
    rt_event = param_ch_event.to_rt_event(8);
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, rt_event.type());
    EXPECT_EQ(8, rt_event.sample_offset());
    EXPECT_EQ(6u, rt_event.parameter_change_event()->processor_id());
    EXPECT_EQ(50u, rt_event.parameter_change_event()->param_id());
    EXPECT_FLOAT_EQ(1.0f, rt_event.parameter_change_event()->value());

    auto string_pro_ch_event = StringPropertyChangeEvent(7, 51, "Hello", IMMEDIATE_PROCESS);
    EXPECT_TRUE(string_pro_ch_event.is_parameter_change_event());
    EXPECT_TRUE(string_pro_ch_event.maps_to_rt_event());
    rt_event = string_pro_ch_event.to_rt_event(10);
    EXPECT_EQ(RtEventType::STRING_PROPERTY_CHANGE, rt_event.type());
    EXPECT_EQ(10, rt_event.sample_offset());
    EXPECT_EQ(7u, rt_event.string_parameter_change_event()->processor_id());
    EXPECT_EQ(51u, rt_event.string_parameter_change_event()->param_id());
    EXPECT_STREQ("Hello", rt_event.string_parameter_change_event()->value()->c_str());

    BlobData testdata = {0, nullptr};
    auto data_pro_ch_event = DataPropertyChangeEvent(8, 52, testdata, IMMEDIATE_PROCESS);
    EXPECT_TRUE(data_pro_ch_event.is_parameter_change_event());
    EXPECT_TRUE(data_pro_ch_event.maps_to_rt_event());
    rt_event = data_pro_ch_event.to_rt_event(10);
    EXPECT_EQ(RtEventType::DATA_PROPERTY_CHANGE, rt_event.type());
    EXPECT_EQ(10, rt_event.sample_offset());
    EXPECT_EQ(8u, rt_event.data_parameter_change_event()->processor_id());
    EXPECT_EQ(52u, rt_event.data_parameter_change_event()->param_id());
    EXPECT_EQ(0, rt_event.data_parameter_change_event()->value().size);
    EXPECT_EQ(nullptr, rt_event.data_parameter_change_event()->value().data);

    auto async_comp_not = AsynchronousProcessorWorkCompletionEvent(123, 9, 53, IMMEDIATE_PROCESS);
    rt_event = async_comp_not.to_rt_event(11);
    EXPECT_EQ(RtEventType::ASYNC_WORK_NOTIFICATION, rt_event.type());
    EXPECT_EQ(123, rt_event.async_work_completion_event()->return_status());
    EXPECT_EQ(9u, rt_event.async_work_completion_event()->processor_id());
    EXPECT_EQ(53u, rt_event.async_work_completion_event()->sending_event_id());

    auto bypass_event = SetProcessorBypassEvent(10, true, IMMEDIATE_PROCESS);
    EXPECT_TRUE(bypass_event.bypass_enabled());
    rt_event = bypass_event.to_rt_event(12);
    EXPECT_EQ(RtEventType::SET_BYPASS, rt_event.type());
    EXPECT_TRUE(rt_event.processor_command_event()->value());
    EXPECT_EQ(10u, rt_event.processor_command_event()->processor_id());
}

TEST(EventTest, TestFromRtEvent)
{
    auto note_on_event = RtEvent::make_note_on_event(2, 0, 1, 48, 1.0f);
    Event* event = Event::from_rt_event(note_on_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_ON, kb_event->subtype());
    EXPECT_EQ(1, kb_event->channel());
    EXPECT_EQ(48, kb_event->note());
    EXPECT_EQ(2u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto note_off_event = RtEvent::make_note_off_event(3, 0, 2, 49, 1.0f);
    event = Event::from_rt_event(note_off_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, kb_event->subtype());
    EXPECT_EQ(2, kb_event->channel());
    EXPECT_EQ(49, kb_event->note());
    EXPECT_EQ(3u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto note_at_event = RtEvent::make_note_aftertouch_event(4, 0, 3, 50, 1.0f);
    event = Event::from_rt_event(note_at_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, kb_event->subtype());
    EXPECT_EQ(3, kb_event->channel());
    EXPECT_EQ(50, kb_event->note());
    EXPECT_EQ(4u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto mod_event = RtEvent::make_kb_modulation_event(5, 0, 4, 0.5f);
    event = Event::from_rt_event(mod_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::MODULATION, kb_event->subtype());
    EXPECT_EQ(4, kb_event->channel());
    EXPECT_EQ(5u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.5f, kb_event->value());
    delete event;

    auto pb_event = RtEvent::make_pitch_bend_event(6, 0, 5, 0.6f);
    event = Event::from_rt_event(pb_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::PITCH_BEND, kb_event->subtype());
    EXPECT_EQ(5, kb_event->channel());
    EXPECT_EQ(6u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.6f, kb_event->value());
    delete event;

    auto at_event = RtEvent::make_aftertouch_event(7, 0, 6, 0.7f);
    event = Event::from_rt_event(at_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::AFTERTOUCH, kb_event->subtype());
    EXPECT_EQ(6, kb_event->channel());
    EXPECT_EQ(7u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.7f, kb_event->value());
    delete event;

    auto midi_event = RtEvent::make_wrapped_midi_event(8, 0, {1,2,3,4});
    event = Event::from_rt_event(midi_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::WRAPPED_MIDI, kb_event->subtype());
    EXPECT_EQ(8u, kb_event->processor_id());
    EXPECT_EQ(MidiDataByte({1,2,3,4}), kb_event->midi_data());
    delete event;

    auto param_ch_event = RtEvent::make_parameter_change_event(9, 0, 50, 0.1f);
    event = Event::from_rt_event(param_ch_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_parameter_change_notification());
    EXPECT_FALSE(event->is_parameter_change_event());
    auto pc_event = static_cast<ParameterChangeNotificationEvent*>(event);
    EXPECT_EQ(ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT, pc_event->subtype());
    EXPECT_EQ(9u, pc_event->processor_id());
    EXPECT_EQ(50u, pc_event->parameter_id());
    EXPECT_FLOAT_EQ(0.1f, pc_event->float_value());
    delete event;

    auto async_work_event = RtEvent::make_async_work_event(dummy_processor_callback, 10, nullptr);
    event = Event::from_rt_event(async_work_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_async_work_event());
    EXPECT_TRUE(event->process_asynchronously());
    delete event;

    BlobData testdata = {0, nullptr};
    auto async_blod_del_event = RtEvent::make_delete_blob_event(testdata);
    event = Event::from_rt_event(async_blod_del_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_async_work_event());
    EXPECT_TRUE(event->process_asynchronously());
    delete event;
}

TEST(EventTest, TestAddProcessorToTrackEventExecution)
{
    // Prepare an engine instance with 1 track
    auto engine = engine::AudioEngine(TEST_SAMPLE_RATE);
    auto processor_container = engine.processor_container();
    auto [status, track_id] = engine.create_track("main", 2);
    ASSERT_EQ(engine::EngineReturnStatus::OK, status);
    auto main_track_id = engine.processor_container()->processor("main")->id();

    auto event = AddProcessorToTrackEvent("gain",
                                          "sushi.testing.gain",
                                          "",
                                          AddProcessorToTrackEvent::ProcessorType::INTERNAL,
                                          main_track_id,
                                          std::nullopt,
                                          IMMEDIATE_PROCESS);

    auto event_status = event.execute(&engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status);
    auto processors = processor_container->processors_on_track(main_track_id);
    ASSERT_EQ(1u, processors.size());
    ASSERT_EQ("gain", processors.front()->name());
}

TEST(EventTest, TestMoveProcessorEventExecution)
{
    // Prepare an engine instance with 2 tracks and 1 processor on track_1
    auto engine = engine::AudioEngine(TEST_SAMPLE_RATE);
    auto processor_container = engine.processor_container();
    engine.create_track("track_1", 2);
    engine.create_track("track_2", 2);
    auto track_1_id = engine.processor_container()->processor("track_1")->id();
    auto track_2_id = engine.processor_container()->processor("track_2")->id();

    auto [proc_status, proc_id] = engine.load_plugin("sushi.testing.gain", "gain", "", engine::PluginType::INTERNAL);
    ASSERT_EQ(engine::EngineReturnStatus::OK, proc_status);
    auto status = engine.add_plugin_to_track(proc_id, track_1_id);
    ASSERT_EQ(engine::EngineReturnStatus::OK, status);
    ASSERT_EQ(1u, processor_container->processors_on_track(track_1_id).size());
    ASSERT_EQ(0u, processor_container->processors_on_track(track_2_id).size());

    // Move it from track_1 to track_2
    auto event = MoveProcessorEvent(proc_id, track_1_id, track_2_id, std::nullopt, IMMEDIATE_PROCESS);
    auto event_status = event.execute(&engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status);

    ASSERT_EQ(0u, processor_container->processors_on_track(track_1_id).size());
    ASSERT_EQ(1u, processor_container->processors_on_track(track_2_id).size());

    // Test with an erroneous target track and see that nothing changed
    auto err_event = MoveProcessorEvent(proc_id, track_2_id, ObjectId(123), std::nullopt, IMMEDIATE_PROCESS);
    event_status = err_event.execute(&engine);
    ASSERT_EQ(MoveProcessorEvent::Status::INVALID_DEST_TRACK, event_status);

    ASSERT_EQ(0u, processor_container->processors_on_track(track_1_id).size());
    ASSERT_EQ(1u, processor_container->processors_on_track(track_2_id).size());
}

TEST(EventTest, TestRemoveProcessorEventExecution)
{
    // Prepare an engine instance with 1 track and 1 processor
    auto engine = engine::AudioEngine(TEST_SAMPLE_RATE);
    auto processor_container = engine.processor_container();
    engine.create_track("main", 2);
    auto main_track_id = engine.processor_container()->processor("main")->id();
    auto [proc_status, proc_id] = engine.load_plugin("sushi.testing.gain", "gain", "", engine::PluginType::INTERNAL);
    ASSERT_EQ(engine::EngineReturnStatus::OK, proc_status);
    auto status = engine.add_plugin_to_track(proc_id, main_track_id);
    ASSERT_EQ(engine::EngineReturnStatus::OK, status);

    auto event = RemoveProcessorEvent(proc_id, main_track_id, IMMEDIATE_PROCESS);
    auto event_status = event.execute(&engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status);

    // Verify that it was deleted
    auto processors = processor_container->processors_on_track(main_track_id);
    ASSERT_EQ(0u, processors.size());
    ASSERT_FALSE(processor_container->processor("gain"));
    ASSERT_FALSE(processor_container->processor(proc_id));
}

TEST(EventTest, TestRemoveTrackEventExecution)
{
    // Prepare an engine instance with 1 track and 1 processor
    auto engine = engine::AudioEngine(TEST_SAMPLE_RATE);
    auto processor_container = engine.processor_container();
    engine.create_track("main", 2);
    auto main_track_id = engine.processor_container()->processor("main")->id();
    auto [proc_status, proc_id] = engine.load_plugin("sushi.testing.gain", "gain", "", engine::PluginType::INTERNAL);
    ASSERT_EQ(engine::EngineReturnStatus::OK, proc_status);
    auto status = engine.add_plugin_to_track(proc_id, main_track_id);
    ASSERT_EQ(engine::EngineReturnStatus::OK, status);

    auto event = RemoveTrackEvent(main_track_id, IMMEDIATE_PROCESS);
    auto event_status = event.execute(&engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status);

    // Verify that the track was deleted together with the processor on it
    ASSERT_EQ(0u, processor_container->all_tracks().size());
    ASSERT_FALSE(processor_container->track("main"));
    ASSERT_FALSE(processor_container->track(main_track_id));
    ASSERT_FALSE(processor_container->processor("gain"));
    ASSERT_FALSE(processor_container->processor(proc_id));
}

// MIDI Controller Event Tests
const MidiDataByte TEST_NOTE_OFF_CH3 = {0x82, 60, 45, 0}; /* Channel 3 */
const MidiDataByte TEST_CTRL_CH_CH4_67 = {0xB3, 67, 75, 0}; /* Channel 4, cc 67 */
const MidiDataByte TEST_CTRL_CH_CH4_68 = {0xB3, 68, 75, 0}; /* Channel 4, cc 68 */
const MidiDataByte TEST_CTRL_CH_CH4_70 = {0xB3, 70, 75, 0}; /* Channel 4, cc 70 */
const MidiDataByte TEST_PRG_CH_CH5 = {0xC4, 40, 0, 0};  /* Channel 5, prg 40 */
const MidiDataByte TEST_PRG_CH_CH6 = {0xC5, 40, 0, 0};  /* Channel 6, prg 40 */
const MidiDataByte TEST_PRG_CH_CH7 = {0xC6, 40, 0, 0};  /* Channel 7, prg 40 */

class MidiControllerEventTestFrontend : public ::testing::Test
{
protected:
    MidiControllerEventTestFrontend() {}

    void SetUp()
    {
        _test_dispatcher = static_cast<EventDispatcherMockup*>(_test_engine.event_dispatcher());
        _midi_dispatcher.set_frontend(&_test_frontend);
    }

    void TearDown()
    {

    }

    EngineMockup _test_engine{TEST_SAMPLE_RATE};
    MidiDispatcher _midi_dispatcher{_test_engine.event_dispatcher(), _test_engine.processor_container()};
    EventDispatcherMockup* _test_dispatcher;
    DummyMidiFrontend _test_frontend;
    sushi::ext::ControlMockup _controller;
};

TEST_F(MidiControllerEventTestFrontend, TestKbdInputConectionDisconnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    const bool raw_midi = false; // Do another with true?

    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 1;

    _midi_dispatcher.set_midi_inputs(5);

    auto connection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                           track_id,
                                                           channel,
                                                           port,
                                                           raw_midi,
                                                           KbdInputToTrackConnectionEvent::Action::Connect,
                                                           IMMEDIATE_PROCESS);

    auto event_status_connect = connection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    auto disconnection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                              track_id,
                                                              channel,
                                                              port,
                                                              raw_midi,
                                                              KbdInputToTrackConnectionEvent::Action::Disconnect,
                                                              IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestKbdInputConectionDisconnectionRaw)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    const bool raw_midi = true; // Do another with true?

    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 1;

    _midi_dispatcher.set_midi_inputs(5);

    auto connection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                           track_id,
                                                           channel,
                                                           port,
                                                           raw_midi,
                                                           KbdInputToTrackConnectionEvent::Action::Connect,
                                                           IMMEDIATE_PROCESS);

    auto event_status_connect = connection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    auto disconnection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                              track_id,
                                                              channel,
                                                              port,
                                                              raw_midi,
                                                              KbdInputToTrackConnectionEvent::Action::Disconnect,
                                                              IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestKbdOutputConectionDisconnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    KeyboardEvent event_ch5(KeyboardEvent::Subtype::NOTE_ON,
                            1,
                            5,
                            48,
                            0.5f,
                            IMMEDIATE_PROCESS);

    /* Send midi message without connections */
    auto status1 = _midi_dispatcher.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status1);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));

    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 0;

    _midi_dispatcher.set_midi_outputs(5);

    auto connection_event = KbdOutputToTrackConnectionEvent(&_midi_dispatcher,
                                                            track_id,
                                                            channel,
                                                            port,
                                                            KbdOutputToTrackConnectionEvent::Action::Connect,
                                                            IMMEDIATE_PROCESS);

    auto event_status_connect = connection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect);

    // TODO/Q: This test fails when running tests in batch, but succeeds when run individually.
    // I introduced it, as I thought it was missing by omission.
    // But maybe there was a reason why sending midi out cannot be tested?
    //auto status2 = _midi_dispatcher.process(&event_ch5);
    //EXPECT_EQ(EventStatus::HANDLED_OK, status2);
    //EXPECT_TRUE(_test_frontend.midi_sent_on_input(0));

    auto disconnection_event = KbdOutputToTrackConnectionEvent(&_midi_dispatcher,
                                                               track_id,
                                                               channel,
                                                               port,
                                                               KbdOutputToTrackConnectionEvent::Action::Disconnect,
                                                               IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    auto status3 = _midi_dispatcher.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status3);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));
}

TEST_F(MidiControllerEventTestFrontend, TestCCDataConnectionDisconnection)
{
    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_4;
    int port = 0;

    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    const std::string parameter_name = "param 1";

    _midi_dispatcher.set_midi_inputs(5);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // Connect CC Number 67:

    auto connect_event1 = ConnectCCToParameterEvent(&_midi_dispatcher,
                                                    processor_id,
                                                    parameter_name,
                                                    channel,
                                                    port,
                                                    67, // cc_number
                                                   0, // min_range
                                                   100, // max_range
                                                   false, // use_relative_mode
                                                   IMMEDIATE_PROCESS);

    auto event_status_connect1 = connect_event1.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect1);

    // Connect CC Number 68:

    auto connect_event2 = ConnectCCToParameterEvent(&_midi_dispatcher,
                                                    processor_id,
                                                    parameter_name,
                                                    channel,
                                                    port,
                                                    68, // cc_number
                                                    0, // min_range
                                                    100, // max_range
                                                    false, // use_relative_mode
                                                    IMMEDIATE_PROCESS);

    auto event_status_connect2 = connect_event2.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect2);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // Connect CC Number 70:

    auto connect_event3 = ConnectCCToParameterEvent(&_midi_dispatcher,
                                                    processor_id,
                                                    parameter_name,
                                                    channel,
                                                    port,
                                                    70, // cc_number
                                                    0, // min_range
                                                    100, // max_range
                                                    false, // use_relative_mode
                                                    IMMEDIATE_PROCESS);

    auto event_status_connect3 = connect_event3.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect3);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect CC Number 67 only:

    auto disconnect_event = DisconnectCCEvent(&_midi_dispatcher,
                                              processor_id,
                                              channel,
                                              port,
                                              67, // cc_number
                                              IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnect_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect all remaining CC's:

    auto disconnect_all_event = DisconnectAllCCFromProcessorEvent(&_midi_dispatcher,
                                                                  processor_id,
                                                                  IMMEDIATE_PROCESS);

    auto event_status_disconnect_all = disconnect_all_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect_all);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestPCDataConnectionDisconnection)
{
    int port = 0;

    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    _midi_dispatcher.set_midi_inputs(5);

    // Connect Channel 5:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto connect_event1 = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                       processor_id,
                                                       sushi::ext::MidiChannel::MIDI_CH_5,
                                                       port,
                                                       PCToProcessorConnectionEvent::Action::Connect,
                                                       IMMEDIATE_PROCESS);

    auto event_status_connect1 = connect_event1.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect1);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Connect Channel 6:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto connect_event2 = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                       processor_id,
                                                       sushi::ext::MidiChannel::MIDI_CH_6,
                                                       port,
                                                       PCToProcessorConnectionEvent::Action::Connect,
                                                       IMMEDIATE_PROCESS);

    auto event_status_connect2 = connect_event2.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect2);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Connect Channel 7:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto connect_event3 = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                       processor_id,
                                               sushi::ext::MidiChannel::MIDI_CH_7,
                                                       port,
                                                       PCToProcessorConnectionEvent::Action::Connect,
                                                       IMMEDIATE_PROCESS);

    auto event_status_connect3 = connect_event3.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect3);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect Channel 5 only:

    auto disconnect_event = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                         processor_id,
                                                         sushi::ext::MidiChannel::MIDI_CH_5,
                                                         port,
                                                         PCToProcessorConnectionEvent::Action::Disconnect,
                                                         IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnect_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect all channels:

    auto disconnect_all_event = DisconnectAllPCFromProcessorEvent(&_midi_dispatcher,
                                                                  processor_id,
                                                                  IMMEDIATE_PROCESS);

    auto event_status_disconnect_all = disconnect_all_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect_all);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}
