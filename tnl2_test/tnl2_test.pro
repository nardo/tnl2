# -------------------------------------------------
# Project created by QtCreator 2010-02-25T18:35:26
# -------------------------------------------------
QT += opengl
TARGET = tnl2_test
TEMPLATE = app
SOURCES += main.cpp \
    tnl2_test_widget.cpp \
    test_game.cpp \
    window.cpp
HEADERS += tnl2_test_widget.h \
    test_player.h \
    test_net_interface.h \
    test_game.h \
    test_connection.h \
    test_building.h \
    ../tnl2/tnl2.h \
    ../tnl2/net_object.h \
    ../tnl2/net_interface.h \
    ../tnl2/net_connection.h \
    ../tnl2/ghost_connection.h \
    ../tnl2/exceptions.h \
    ../tnl2/event_connection.h \
    window.h \
    ../../torque_sockets/core/zone_allocator.h \
    ../../torque_sockets/core/utils.h \
    ../../torque_sockets/core/type_record.h \
    ../../torque_sockets/core/type_database.h \
    ../../torque_sockets/core/thread_queue.h \
    ../../torque_sockets/core/thread.h \
    ../../torque_sockets/core/system_includes_win32.h \
    ../../torque_sockets/core/system_includes_mac_osx.h \
    ../../torque_sockets/core/system_includes_linux.h \
    ../../torque_sockets/core/string.h \
    ../../torque_sockets/core/static_to_indexed_string_map.h \
    ../../torque_sockets/core/small_block_allocator.h \
    ../../torque_sockets/core/ref_object.h \
    ../../torque_sockets/core/power_two_functions.h \
    ../../torque_sockets/core/platform.h \
    ../../torque_sockets/core/page_allocator.h \
    ../../torque_sockets/core/memory_functions.h \
    ../../torque_sockets/core/log.h \
    ../../torque_sockets/core/indexed_string.h \
    ../../torque_sockets/core/hash_table_flat.h \
    ../../torque_sockets/core/hash_table_array.h \
    ../../torque_sockets/core/hash.h \
    ../../torque_sockets/core/functor.h \
    ../../torque_sockets/core/function_record.h \
    ../../torque_sockets/core/formatted_string_buffer.h \
    ../../torque_sockets/core/dictionary.h \
    ../../torque_sockets/core/cpu_endian.h \
    ../../torque_sockets/core/core.h \
    ../../torque_sockets/core/context.h \
    ../../torque_sockets/core/construct.h \
    ../../torque_sockets/core/byte_stream_fixed.h \
    ../../torque_sockets/core/byte_buffer.h \
    ../../torque_sockets/core/bit_stream.h \
    ../../torque_sockets/core/base_types.h \
    ../../torque_sockets/core/base_type_traits.h \
    ../../torque_sockets/core/base_type_io.h \
    ../../torque_sockets/core/base_type_declarations.h \
    ../../torque_sockets/core/assert.h \
    ../../torque_sockets/core/array.h \
    ../../torque_sockets/core/algorithm_templates.h \
    ../../torque_sockets/torque_sockets/udp_socket.h \
    ../../torque_sockets/torque_sockets/torque_sockets.h \
    ../../torque_sockets/torque_sockets/torque_socket.h \
    ../../torque_sockets/torque_sockets/torque_connection.h \
    ../../torque_sockets/torque_sockets/time.h \
    ../../torque_sockets/torque_sockets/symmetric_cipher.h \
    ../../torque_sockets/torque_sockets/sockets.h \
    ../../torque_sockets/torque_sockets/random_generator.h \
    ../../torque_sockets/torque_sockets/packet_stream.h \
    ../../torque_sockets/torque_sockets/nonce.h \
    ../../torque_sockets/torque_sockets/client_puzzle.h \
    ../../torque_sockets/torque_sockets/buffer_utils.h \
    ../../torque_sockets/torque_sockets/asymmetric_key.h \
    ../../torque_sockets/torque_sockets/address.h \
    ../../torque_sockets/torque_sockets/pending_connection.h \
    ../../torque_sockets/torque_sockets/torque_sockets_c_api.h \
    ../../torque_sockets/torque_sockets_reference_api.h \
    ../../torque_sockets/torque_sockets/torque_sockets_c_implementation.h
FORMS += 
INCLUDEPATH += ../tnl2/ \
	../../torque_sockets/ \
	../../torque_sockets/lib/libtomcrypt/src/headers
LIBS += -L../../torque_sockets/lib/libtomcrypt \
    -L../../torque_sockets/lib/libtommath \
    -ltomcrypt \
    -ltommath \
    -framework \
    CoreServices
