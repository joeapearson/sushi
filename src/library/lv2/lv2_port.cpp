/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - port.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include <math.h>

#include "lv2_model.h"
#include "lv2_worker.h"
#include "logging.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

// TODO: Pass model as const eventually
Port::Port(const LilvPlugin *plugin, int port_index, float default_value, LV2Model* model):
control(0.0f),
_flow(FLOW_UNKNOWN),
_evbuf(nullptr), // For MIDI ports, otherwise NULL
_buf_size(0), // Custom buffer size, or 0
_index(port_index)
{
    lilv_port = lilv_plugin_get_port_by_index(plugin, port_index);

    const bool optional = lilv_port_has_property(plugin, lilv_port, model->get_nodes().lv2_connectionOptional);

    /* Set the port flow (input or output) */
    if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_InputPort))
    {
        _flow = FLOW_INPUT;
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_OutputPort))
    {
        _flow = FLOW_OUTPUT;
    }
    else if (!optional)
    {
        assert(false);
        SUSHI_LOG_ERROR("Mandatory LV2 port has unknown type (neither input nor output)");
        throw Port::FailedCreation();
    }

    const bool hidden = !_show_hidden &&
                        lilv_port_has_property(plugin, lilv_port, model->get_nodes().pprops_notOnGUI);

    /* Set control values */
    if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_ControlPort))
    {
        _type = TYPE_CONTROL;

// TODO: min max def are used also in the Jalv control structure. Remove these eventually?
        LilvNode* minNode;
        LilvNode* maxNode;
        LilvNode* defNode;

        lilv_port_get_range(plugin, lilv_port, &defNode, &minNode, &maxNode);

        if(defNode != nullptr)
            def = lilv_node_as_float(defNode);

        if(maxNode != nullptr)
            max = lilv_node_as_float(maxNode);

        if(minNode != nullptr)
            min = lilv_node_as_float(minNode);

        control = isnan(default_value) ? def : default_value;

        lilv_node_free(minNode);
        lilv_node_free(maxNode);
        lilv_node_free(defNode);

        if (!hidden)
        {
            model->controls.emplace_back(new_port_control(this, model, _index));
        }
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_AudioPort))
    {
        _type = TYPE_AUDIO;

// TODO: CV port(s).
//#ifdef HAVE_JACK_METADATA
//        } else if (lilv_port_is_a(model->plugin, port->lilv_port,
//                      model->nodes.lv2_CVPort)) {
//port->type = TYPE_CV;
//#endif

    }
    else if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().atom_AtomPort))
    {
        _type = TYPE_EVENT;
    }
    else if (!optional)
    {
        assert(false);
        SUSHI_LOG_ERROR("Mandatory LV2 port has unknown data type");
        throw Port::FailedCreation();
    }

    if (!model->buf_size_set) {
        _allocate_port_buffers(model);
    }
}

void Port::resetInputBuffer()
{
    lv2_evbuf_reset(_evbuf, true);
}

void Port::resetOutputBuffer()
{
    lv2_evbuf_reset(_evbuf, false);
}

void Port::_allocate_port_buffers(LV2Model* model)
{
    switch (_type)
    {
        case TYPE_EVENT:
        {
            lv2_evbuf_free(_evbuf);

            auto handle = model->get_map().handle;

            _evbuf = lv2_evbuf_new(
                    model->get_midi_buffer_size(),
                    model->get_map().map(handle, lilv_node_as_string(model->get_nodes().atom_Chunk)),
                    model->get_map().map(handle, lilv_node_as_string(model->get_nodes().atom_Sequence)));

            lilv_instance_connect_port(
                    model->get_plugin_instance(), _index, lv2_evbuf_get_buffer(_evbuf));
        }
        default:
            break;
    }
}

}
}

#endif //SUSHI_BUILD_WITH_LV2