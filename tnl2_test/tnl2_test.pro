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
    ../../torque_sockets/platform_library/core/zone_allocator.h \
    ../../torque_sockets/platform_library/core/utils.h \
    ../../torque_sockets/platform_library/core/type_record.h \
    ../../torque_sockets/platform_library/core/type_database.h \
    ../../torque_sockets/platform_library/core/thread_queue.h \
    ../../torque_sockets/platform_library/core/thread.h \
    ../../torque_sockets/platform_library/core/string.h \
    ../../torque_sockets/platform_library/core/static_to_indexed_string_map.h \
    ../../torque_sockets/platform_library/core/small_block_allocator.h \
    ../../torque_sockets/platform_library/core/ref_object.h \
    ../../torque_sockets/platform_library/core/power_two_functions.h \
    ../../torque_sockets/platform_library/core/page_allocator.h \
    ../../torque_sockets/platform_library/core/memory_functions.h \
    ../../torque_sockets/platform_library/core/log.h \
    ../../torque_sockets/platform_library/core/indexed_string.h \
    ../../torque_sockets/platform_library/core/hash_table_flat.h \
    ../../torque_sockets/platform_library/core/hash_table_array.h \
    ../../torque_sockets/platform_library/core/hash.h \
    ../../torque_sockets/platform_library/core/functor.h \
    ../../torque_sockets/platform_library/core/function_record.h \
    ../../torque_sockets/platform_library/core/formatted_string_buffer.h \
    ../../torque_sockets/platform_library/core/dictionary.h \
    ../../torque_sockets/platform_library/core/cpu_endian.h \
    ../../torque_sockets/platform_library/core/core.h \
    ../../torque_sockets/platform_library/core/context.h \
    ../../torque_sockets/platform_library/core/construct.h \
    ../../torque_sockets/platform_library/core/byte_stream_fixed.h \
    ../../torque_sockets/platform_library/core/byte_buffer.h \
    ../../torque_sockets/platform_library/core/bit_stream.h \
    ../../torque_sockets/platform_library/core/base_types.h \
    ../../torque_sockets/platform_library/core/base_type_traits.h \
    ../../torque_sockets/platform_library/core/base_type_io.h \
    ../../torque_sockets/platform_library/core/base_type_declarations.h \
    ../../torque_sockets/platform_library/core/assert.h \
    ../../torque_sockets/platform_library/core/array.h \
    ../../torque_sockets/platform_library/core/algorithm_templates.h \
    ../../torque_sockets/platform_library/net/udp_socket.h \
    ../../torque_sockets/platform_library/net/time.h \
    ../../torque_sockets/platform_library/net/symmetric_cipher.h \
    ../../torque_sockets/platform_library/net/sockets.h \
    ../../torque_sockets/platform_library/net/random_generator.h \
    ../../torque_sockets/platform_library/net/packet_stream.h \
    ../../torque_sockets/platform_library/net/nonce.h \
    ../../torque_sockets/platform_library/net/net.h \
    ../../torque_sockets/platform_library/net/interface.h \
    ../../torque_sockets/platform_library/net/connection.h \
    ../../torque_sockets/platform_library/net/client_puzzle.h \
    ../../torque_sockets/platform_library/net/buffer_utils.h \
    ../../torque_sockets/platform_library/net/asymmetric_key.h \
    ../../torque_sockets/platform_library/net/address.h \
    ../../torque_sockets/platform_library/platform/system_includes_win32.h \
    ../../torque_sockets/platform_library/platform/system_includes_mac_osx.h \
    ../../torque_sockets/platform_library/platform/system_includes_linux.h \
    ../../torque_sockets/platform_library/platform/platform.h \
    ../../torque_sockets/platform_library/torque_sockets_implementation.h \
    ../tnl2/tnl2.h \
    ../tnl2/net_object.h \
    ../tnl2/net_interface.h \
    ../tnl2/net_connection.h \
    ../tnl2/ghost_connection.h \
    ../tnl2/exceptions.h \
    ../tnl2/event_connection.h \
    window.h
FORMS += 
INCLUDEPATH += ../tnl2 \
    ../../torque_sockets/platform_library \
    ../../torque_sockets/lib/libtomcrypt/src/headers
LIBS += -L../../torque_sockets/lib/libtomcrypt \
    -L../../torque_sockets/lib/libtommath \
    -ltomcrypt \
    -ltommath \
    -framework \
    CoreServices
