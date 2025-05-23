// TODO: list
// need to work on the indexing on the file selection list using modulo and min of max index

#include <algorithm>
#include <fmt/core.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nlohmann/detail/input/input_adapters.hpp>

#include "modal_editor/modal_editor.hpp"

#include "graphics/ui/ui.hpp"
#include "graphics/window/window.hpp"
#include "graphics/shader_cache/shader_cache.hpp"
#include "graphics/batcher/generated/batcher.hpp"
#include "graphics/vertex_geometry/vertex_geometry.hpp"
#include "graphics/texture_atlas/texture_atlas.hpp"
#include "graphics/viewport/viewport.hpp"
#include "graphics/colors/colors.hpp"

#include "utility/fs_utils/fs_utils.hpp"
#include "utility/input_state/input_state.hpp"
#include "utility/glfw_lambda_callback_manager/glfw_lambda_callback_manager.hpp"
#include "utility/limited_vector/limited_vector.hpp"
#include "utility/temporal_binary_signal/temporal_binary_signal.hpp"
#include "utility/text_buffer/text_buffer.hpp"
#include "utility/input_state/input_state.hpp"
#include "utility/periodic_signal/periodic_signal.hpp"
#include "utility/config_file_parser/config_file_parser.hpp"
#include "utility/lsp_client/lsp_client.hpp"
#include "utility/resource_path/resource_path.hpp"
#include "utility/text_diff/text_diff.hpp"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <string>
#include <stdexcept>

#include <iostream>
#include <filesystem>
#include <cstdio>

#include <fstream>
#include <iostream>
#include <stddef.h> // for size_t
#include <string>   // for char_traits, operator+, string, basic_string, to_string
#include <vector>   // for vector

#include "ftxui/component/component.hpp" // for CatchEvent, Renderer
#include "ftxui/component/event.hpp"     // for Event
#include "ftxui/component/mouse.hpp" // for Mouse, Mouse::Left, Mouse::Middle, Mouse::None, Mouse::Pressed, Mouse::Released, Mouse::Right, Mouse::WheelDown, Mouse::WheelUp
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/elements.hpp"                 // for text, vbox, window, Element, Elements

#include <ftxui/dom/elements.hpp>  // for Fit, canvas, operator|, border, Element
#include <ftxui/screen/screen.hpp> // for Pixel, Screen

#include "ftxui/dom/canvas.hpp"   // for Canvas
#include "ftxui/screen/color.hpp" // for Color, Color::Red, Color::Blue, Color::Green, ftxui

Colors colors;
/**
 * Gets the directory of the executable.
 *
 * @return A string representing the directory where the executable is located.
 */
std::string get_executable_path(char **argv) {
    auto dir = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    return dir.string(); // Returns the directory as a string
}

std::string extract_filename(const std::string &full_path) {
    std::filesystem::path path(full_path);
    return path.filename().string();
}

std::string get_current_time_string() {
    // Get current time as a time_t object
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // Convert to local time
    std::tm local_tm = *std::localtime(&now_time_t);

    // Format time to string
    std::ostringstream time_stream;
    time_stream << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S"); // Custom format
    return time_stream.str();
}

std::string remove_file_prefix(const std::string &file_uri) {
#ifdef _WIN32
    // On Windows, remove "file:///" prefix if present
    return (file_uri.rfind("file:///", 0) == 0) ? file_uri.substr(8) : file_uri;
#else
    // On other platforms, remove "file://" prefix if present
    return (file_uri.rfind("file://", 0) == 0) ? file_uri.substr(7) : file_uri;
#endif
}

bool starts_with_any_prefix(const std::string &str, const std::vector<std::string> &prefixes) {
    for (const auto &prefix : prefixes) {
        if (str.rfind(prefix, 0) == 0) { // Check if 'prefix' is at position 0
            return true;
        }
    }
    return false;
}

void go_to_definition(JSON lsp_response, Viewport &viewport, LSPClient &lsp_client) {
    try {
        if (!lsp_response.contains("result") || lsp_response["result"].empty()) {
            std::cerr << "LSP go_to_definition: No result found in response." << std::endl;
            return;
        }

        JSON location = lsp_response["result"].is_array() ? lsp_response["result"][0] : lsp_response["result"];

        if (!location.contains("uri") || !location.contains("range")) {
            std::cerr << "LSP go_to_definition: Missing uri or range in response." << std::endl;
            return;
        }

        std::string file_uri = location["uri"].get<std::string>();

        std::string file_to_open = remove_file_prefix(file_uri);

        int line = location["range"]["start"]["line"].get<int>();
        int col = location["range"]["start"]["character"].get<int>();

        // switch_files(viewport, file_to_open, true);
        viewport.set_active_buffer_line_col_under_cursor(line, col);
    } catch (const std::exception &e) {
        std::cerr << "LSP go_to_definition: Exception parsing response: " << e.what() << std::endl;
    }
}

// this is disabled because I don't want to do lsp just yet
void go_to_definition_dummy() {
    // if (modal_editor.current_mode == MOVE_AND_EDIT) {
    //     auto on_definition_found = [&](JSON lsp_response) {
    //         // NOTE: that lsp_response needs to be captured by value because it will go out of scope after this
    //         // function is called
    //         auto callback = [lsp_response, &viewport, &lsp_client]() {
    //             go_to_definition(lsp_response, viewport, lsp_client);
    //         };
    //         lsp_callbacks_to_run_synchronously.push_back(callback);
    //     };
    //
    //     lsp_client.make_go_to_definition_request(viewport.buffer->current_file_path,
    //                                              viewport.active_buffer_line_under_cursor,
    //                                              viewport.active_buffer_col_under_cursor, on_definition_found);
    //
    //     auto tr =
    //         TextRange(viewport.active_buffer_line_under_cursor, 0, viewport.active_buffer_line_under_cursor,
    //                   viewport.buffer->get_line(viewport.active_buffer_line_under_cursor).length());
    //
    //     /*lsp_client.make_get_text_request(viewport.buffer->current_file_path, tr);*/
    //     }
}

// code to make sure the cursor stays within the lines
void snap_to_end_of_line_while_navigating(Viewport &viewport, int &saved, int &saved_last_col, int &saved_last_line) {

    const std::string &line = viewport.buffer->get_line(viewport.active_buffer_line_under_cursor);

    if (line.size() < saved_last_col) {
        if (saved == 0) {
            saved_last_col = viewport.active_buffer_col_under_cursor;
            saved = 1;
            viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, line.size());
        }
    }
    if (line.size() >= saved_last_col) {
        if (saved == 1) {
            viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, saved_last_col);
        }
        saved_last_col = viewport.active_buffer_col_under_cursor;
        saved = 0;
    }

    if (saved_last_line != viewport.active_buffer_line_under_cursor && saved == 1) {
        viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, line.size());
        saved_last_line = viewport.active_buffer_line_under_cursor;
    }

    if (line.size() != viewport.active_buffer_col_under_cursor) {
        saved = 0;
        saved_last_col = viewport.active_buffer_col_under_cursor;
    }
}

bool is_integer(const std::string &str) {
    // create a stringstream from the input string
    std::stringstream ss(str);
    // declare a temporary integer variable to hold the value
    int temp;
    // try to extract an integer from the stringstream and ensure the whole string was processed
    return (ss >> temp && ss.eof());
}

// deprecated in terminal verison
InputKeyState create_input_key_state(InputState &input_state) {
    InputKeyState iks;

    iks.input_key_to_is_pressed[InputKey::a] = input_state.is_pressed(EKey::a);
    iks.input_key_to_is_pressed[InputKey::b] = input_state.is_pressed(EKey::b);
    iks.input_key_to_is_pressed[InputKey::c] = input_state.is_pressed(EKey::c);
    iks.input_key_to_is_pressed[InputKey::d] = input_state.is_pressed(EKey::d);
    iks.input_key_to_is_pressed[InputKey::e] = input_state.is_pressed(EKey::e);
    iks.input_key_to_is_pressed[InputKey::f] = input_state.is_pressed(EKey::f);
    iks.input_key_to_is_pressed[InputKey::g] = input_state.is_pressed(EKey::g);
    iks.input_key_to_is_pressed[InputKey::h] = input_state.is_pressed(EKey::h);
    iks.input_key_to_is_pressed[InputKey::i] = input_state.is_pressed(EKey::i);
    iks.input_key_to_is_pressed[InputKey::j] = input_state.is_pressed(EKey::j);
    iks.input_key_to_is_pressed[InputKey::k] = input_state.is_pressed(EKey::k);
    iks.input_key_to_is_pressed[InputKey::l] = input_state.is_pressed(EKey::l);
    iks.input_key_to_is_pressed[InputKey::m] = input_state.is_pressed(EKey::m);
    iks.input_key_to_is_pressed[InputKey::n] = input_state.is_pressed(EKey::n);
    iks.input_key_to_is_pressed[InputKey::o] = input_state.is_pressed(EKey::o);
    iks.input_key_to_is_pressed[InputKey::p] = input_state.is_pressed(EKey::p);
    iks.input_key_to_is_pressed[InputKey::q] = input_state.is_pressed(EKey::q);
    iks.input_key_to_is_pressed[InputKey::r] = input_state.is_pressed(EKey::r);
    iks.input_key_to_is_pressed[InputKey::s] = input_state.is_pressed(EKey::s);
    iks.input_key_to_is_pressed[InputKey::t] = input_state.is_pressed(EKey::t);
    iks.input_key_to_is_pressed[InputKey::u] = input_state.is_pressed(EKey::u);
    iks.input_key_to_is_pressed[InputKey::v] = input_state.is_pressed(EKey::v);
    iks.input_key_to_is_pressed[InputKey::w] = input_state.is_pressed(EKey::w);
    iks.input_key_to_is_pressed[InputKey::x] = input_state.is_pressed(EKey::x);
    iks.input_key_to_is_pressed[InputKey::y] = input_state.is_pressed(EKey::y);
    iks.input_key_to_is_pressed[InputKey::z] = input_state.is_pressed(EKey::z);

    iks.input_key_to_is_pressed[InputKey::SPACE] = input_state.is_pressed(EKey::SPACE);
    iks.input_key_to_is_pressed[InputKey::GRAVE_ACCENT] = input_state.is_pressed(EKey::GRAVE_ACCENT);
    iks.input_key_to_is_pressed[InputKey::TILDE] = input_state.is_pressed(EKey::TILDE);

    iks.input_key_to_is_pressed[InputKey::ONE] = input_state.is_pressed(EKey::ONE);
    iks.input_key_to_is_pressed[InputKey::TWO] = input_state.is_pressed(EKey::TWO);
    iks.input_key_to_is_pressed[InputKey::THREE] = input_state.is_pressed(EKey::THREE);
    iks.input_key_to_is_pressed[InputKey::FOUR] = input_state.is_pressed(EKey::FOUR);
    iks.input_key_to_is_pressed[InputKey::FIVE] = input_state.is_pressed(EKey::FIVE);
    iks.input_key_to_is_pressed[InputKey::SIX] = input_state.is_pressed(EKey::SIX);
    iks.input_key_to_is_pressed[InputKey::SEVEN] = input_state.is_pressed(EKey::SEVEN);
    iks.input_key_to_is_pressed[InputKey::EIGHT] = input_state.is_pressed(EKey::EIGHT);
    iks.input_key_to_is_pressed[InputKey::NINE] = input_state.is_pressed(EKey::NINE);
    iks.input_key_to_is_pressed[InputKey::ZERO] = input_state.is_pressed(EKey::ZERO);
    iks.input_key_to_is_pressed[InputKey::MINUS] = input_state.is_pressed(EKey::MINUS);
    iks.input_key_to_is_pressed[InputKey::EQUAL] = input_state.is_pressed(EKey::EQUAL);

    iks.input_key_to_is_pressed[InputKey::EXCLAMATION_POINT] = input_state.is_pressed(EKey::EXCLAMATION_POINT);
    iks.input_key_to_is_pressed[InputKey::AT_SIGN] = input_state.is_pressed(EKey::AT_SIGN);
    iks.input_key_to_is_pressed[InputKey::NUMBER_SIGN] = input_state.is_pressed(EKey::NUMBER_SIGN);
    iks.input_key_to_is_pressed[InputKey::DOLLAR_SIGN] = input_state.is_pressed(EKey::DOLLAR_SIGN);
    iks.input_key_to_is_pressed[InputKey::PERCENT_SIGN] = input_state.is_pressed(EKey::PERCENT_SIGN);
    iks.input_key_to_is_pressed[InputKey::CARET] = input_state.is_pressed(EKey::CARET);
    iks.input_key_to_is_pressed[InputKey::AMPERSAND] = input_state.is_pressed(EKey::AMPERSAND);
    iks.input_key_to_is_pressed[InputKey::ASTERISK] = input_state.is_pressed(EKey::ASTERISK);
    iks.input_key_to_is_pressed[InputKey::LEFT_PARENTHESIS] = input_state.is_pressed(EKey::LEFT_PARENTHESIS);
    iks.input_key_to_is_pressed[InputKey::RIGHT_PARENTHESIS] = input_state.is_pressed(EKey::RIGHT_PARENTHESIS);
    iks.input_key_to_is_pressed[InputKey::UNDERSCORE] = input_state.is_pressed(EKey::UNDERSCORE);
    iks.input_key_to_is_pressed[InputKey::PLUS] = input_state.is_pressed(EKey::PLUS);

    iks.input_key_to_is_pressed[InputKey::LEFT_SQUARE_BRACKET] = input_state.is_pressed(EKey::LEFT_SQUARE_BRACKET);
    iks.input_key_to_is_pressed[InputKey::RIGHT_SQUARE_BRACKET] = input_state.is_pressed(EKey::RIGHT_SQUARE_BRACKET);
    iks.input_key_to_is_pressed[InputKey::LEFT_CURLY_BRACKET] = input_state.is_pressed(EKey::LEFT_CURLY_BRACKET);
    iks.input_key_to_is_pressed[InputKey::RIGHT_CURLY_BRACKET] = input_state.is_pressed(EKey::RIGHT_CURLY_BRACKET);

    iks.input_key_to_is_pressed[InputKey::COMMA] = input_state.is_pressed(EKey::COMMA);
    iks.input_key_to_is_pressed[InputKey::PERIOD] = input_state.is_pressed(EKey::PERIOD);
    iks.input_key_to_is_pressed[InputKey::LESS_THAN] = input_state.is_pressed(EKey::LESS_THAN);
    iks.input_key_to_is_pressed[InputKey::GREATER_THAN] = input_state.is_pressed(EKey::GREATER_THAN);

    iks.input_key_to_is_pressed[InputKey::CAPS_LOCK] = input_state.is_pressed(EKey::CAPS_LOCK);
    iks.input_key_to_is_pressed[InputKey::ESCAPE] = input_state.is_pressed(EKey::ESCAPE);
    iks.input_key_to_is_pressed[InputKey::ENTER] = input_state.is_pressed(EKey::ENTER);
    iks.input_key_to_is_pressed[InputKey::TAB] = input_state.is_pressed(EKey::TAB);
    iks.input_key_to_is_pressed[InputKey::BACKSPACE] = input_state.is_pressed(EKey::BACKSPACE);
    iks.input_key_to_is_pressed[InputKey::INSERT] = input_state.is_pressed(EKey::INSERT);
    iks.input_key_to_is_pressed[InputKey::DELETE] = input_state.is_pressed(EKey::DELETE);

    iks.input_key_to_is_pressed[InputKey::RIGHT] = input_state.is_pressed(EKey::RIGHT);
    iks.input_key_to_is_pressed[InputKey::LEFT] = input_state.is_pressed(EKey::LEFT);
    iks.input_key_to_is_pressed[InputKey::UP] = input_state.is_pressed(EKey::UP);
    iks.input_key_to_is_pressed[InputKey::DOWN] = input_state.is_pressed(EKey::DOWN);

    iks.input_key_to_is_pressed[InputKey::SLASH] = input_state.is_pressed(EKey::SLASH);
    iks.input_key_to_is_pressed[InputKey::QUESTION_MARK] = input_state.is_pressed(EKey::QUESTION_MARK);
    iks.input_key_to_is_pressed[InputKey::BACKSLASH] = input_state.is_pressed(EKey::BACKSLASH);
    iks.input_key_to_is_pressed[InputKey::PIPE] = input_state.is_pressed(EKey::PIPE);
    iks.input_key_to_is_pressed[InputKey::COLON] = input_state.is_pressed(EKey::COLON);
    iks.input_key_to_is_pressed[InputKey::SEMICOLON] = input_state.is_pressed(EKey::SEMICOLON);
    iks.input_key_to_is_pressed[InputKey::SINGLE_QUOTE] = input_state.is_pressed(EKey::SINGLE_QUOTE);
    iks.input_key_to_is_pressed[InputKey::DOUBLE_QUOTE] = input_state.is_pressed(EKey::DOUBLE_QUOTE);

    iks.input_key_to_is_pressed[InputKey::LEFT_SHIFT] = input_state.is_pressed(EKey::LEFT_SHIFT);
    iks.input_key_to_is_pressed[InputKey::RIGHT_SHIFT] = input_state.is_pressed(EKey::RIGHT_SHIFT);
    iks.input_key_to_is_pressed[InputKey::LEFT_CONTROL] = input_state.is_pressed(EKey::LEFT_CONTROL);
    iks.input_key_to_is_pressed[InputKey::RIGHT_CONTROL] = input_state.is_pressed(EKey::RIGHT_CONTROL);
    iks.input_key_to_is_pressed[InputKey::LEFT_ALT] = input_state.is_pressed(EKey::LEFT_ALT);
    iks.input_key_to_is_pressed[InputKey::RIGHT_ALT] = input_state.is_pressed(EKey::RIGHT_ALT);
    iks.input_key_to_is_pressed[InputKey::LEFT_SUPER] = input_state.is_pressed(EKey::LEFT_SUPER);
    iks.input_key_to_is_pressed[InputKey::RIGHT_SUPER] = input_state.is_pressed(EKey::RIGHT_SUPER);

    iks.input_key_to_is_pressed[InputKey::FUNCTION_KEY] = input_state.is_pressed(EKey::FUNCTION_KEY);
    iks.input_key_to_is_pressed[InputKey::MENU_KEY] = input_state.is_pressed(EKey::MENU_KEY);

    iks.input_key_to_is_pressed[InputKey::LEFT_MOUSE_BUTTON] = input_state.is_pressed(EKey::LEFT_MOUSE_BUTTON);
    iks.input_key_to_is_pressed[InputKey::RIGHT_MOUSE_BUTTON] = input_state.is_pressed(EKey::RIGHT_MOUSE_BUTTON);
    iks.input_key_to_is_pressed[InputKey::MIDDLE_MOUSE_BUTTON] = input_state.is_pressed(EKey::MIDDLE_MOUSE_BUTTON);
    iks.input_key_to_is_pressed[InputKey::SCROLL_UP] = input_state.is_pressed(EKey::SCROLL_UP);
    // NOTE: the following one is causing out of range, instead of bug fixing this, I'm just not going to because we're
    // probably not going to use scroll for a while and fix the bug later
    // iks.input_key_to_is_pressed[InputKey::SCROLL_DOWN] = input_state.is_pressed(EKey::SCROLL_DOWN);

    // not required
    // iks.input_key_to_is_pressed[InputKey::DUMMY] = input_state.is_pressed(EKey::DUMMY);

    iks.input_key_to_just_pressed[InputKey::a] = input_state.is_just_pressed(EKey::a);
    iks.input_key_to_just_pressed[InputKey::b] = input_state.is_just_pressed(EKey::b);
    iks.input_key_to_just_pressed[InputKey::c] = input_state.is_just_pressed(EKey::c);
    iks.input_key_to_just_pressed[InputKey::d] = input_state.is_just_pressed(EKey::d);
    iks.input_key_to_just_pressed[InputKey::e] = input_state.is_just_pressed(EKey::e);
    iks.input_key_to_just_pressed[InputKey::f] = input_state.is_just_pressed(EKey::f);
    iks.input_key_to_just_pressed[InputKey::g] = input_state.is_just_pressed(EKey::g);
    iks.input_key_to_just_pressed[InputKey::h] = input_state.is_just_pressed(EKey::h);
    iks.input_key_to_just_pressed[InputKey::i] = input_state.is_just_pressed(EKey::i);
    iks.input_key_to_just_pressed[InputKey::j] = input_state.is_just_pressed(EKey::j);
    iks.input_key_to_just_pressed[InputKey::k] = input_state.is_just_pressed(EKey::k);
    iks.input_key_to_just_pressed[InputKey::l] = input_state.is_just_pressed(EKey::l);
    iks.input_key_to_just_pressed[InputKey::m] = input_state.is_just_pressed(EKey::m);
    iks.input_key_to_just_pressed[InputKey::n] = input_state.is_just_pressed(EKey::n);
    iks.input_key_to_just_pressed[InputKey::o] = input_state.is_just_pressed(EKey::o);
    iks.input_key_to_just_pressed[InputKey::p] = input_state.is_just_pressed(EKey::p);
    iks.input_key_to_just_pressed[InputKey::q] = input_state.is_just_pressed(EKey::q);
    iks.input_key_to_just_pressed[InputKey::r] = input_state.is_just_pressed(EKey::r);
    iks.input_key_to_just_pressed[InputKey::s] = input_state.is_just_pressed(EKey::s);
    iks.input_key_to_just_pressed[InputKey::t] = input_state.is_just_pressed(EKey::t);
    iks.input_key_to_just_pressed[InputKey::u] = input_state.is_just_pressed(EKey::u);
    iks.input_key_to_just_pressed[InputKey::v] = input_state.is_just_pressed(EKey::v);
    iks.input_key_to_just_pressed[InputKey::w] = input_state.is_just_pressed(EKey::w);
    iks.input_key_to_just_pressed[InputKey::x] = input_state.is_just_pressed(EKey::x);
    iks.input_key_to_just_pressed[InputKey::y] = input_state.is_just_pressed(EKey::y);
    iks.input_key_to_just_pressed[InputKey::z] = input_state.is_just_pressed(EKey::z);

    iks.input_key_to_just_pressed[InputKey::SPACE] = input_state.is_just_pressed(EKey::SPACE);
    iks.input_key_to_just_pressed[InputKey::GRAVE_ACCENT] = input_state.is_just_pressed(EKey::GRAVE_ACCENT);
    iks.input_key_to_just_pressed[InputKey::TILDE] = input_state.is_just_pressed(EKey::TILDE);

    iks.input_key_to_just_pressed[InputKey::ONE] = input_state.is_just_pressed(EKey::ONE);
    iks.input_key_to_just_pressed[InputKey::TWO] = input_state.is_just_pressed(EKey::TWO);
    iks.input_key_to_just_pressed[InputKey::THREE] = input_state.is_just_pressed(EKey::THREE);
    iks.input_key_to_just_pressed[InputKey::FOUR] = input_state.is_just_pressed(EKey::FOUR);
    iks.input_key_to_just_pressed[InputKey::FIVE] = input_state.is_just_pressed(EKey::FIVE);
    iks.input_key_to_just_pressed[InputKey::SIX] = input_state.is_just_pressed(EKey::SIX);
    iks.input_key_to_just_pressed[InputKey::SEVEN] = input_state.is_just_pressed(EKey::SEVEN);
    iks.input_key_to_just_pressed[InputKey::EIGHT] = input_state.is_just_pressed(EKey::EIGHT);
    iks.input_key_to_just_pressed[InputKey::NINE] = input_state.is_just_pressed(EKey::NINE);
    iks.input_key_to_just_pressed[InputKey::ZERO] = input_state.is_just_pressed(EKey::ZERO);
    iks.input_key_to_just_pressed[InputKey::MINUS] = input_state.is_just_pressed(EKey::MINUS);
    iks.input_key_to_just_pressed[InputKey::EQUAL] = input_state.is_just_pressed(EKey::EQUAL);

    iks.input_key_to_just_pressed[InputKey::EXCLAMATION_POINT] = input_state.is_just_pressed(EKey::EXCLAMATION_POINT);
    iks.input_key_to_just_pressed[InputKey::AT_SIGN] = input_state.is_just_pressed(EKey::AT_SIGN);
    iks.input_key_to_just_pressed[InputKey::NUMBER_SIGN] = input_state.is_just_pressed(EKey::NUMBER_SIGN);
    iks.input_key_to_just_pressed[InputKey::DOLLAR_SIGN] = input_state.is_just_pressed(EKey::DOLLAR_SIGN);
    iks.input_key_to_just_pressed[InputKey::PERCENT_SIGN] = input_state.is_just_pressed(EKey::PERCENT_SIGN);
    iks.input_key_to_just_pressed[InputKey::CARET] = input_state.is_just_pressed(EKey::CARET);
    iks.input_key_to_just_pressed[InputKey::AMPERSAND] = input_state.is_just_pressed(EKey::AMPERSAND);
    iks.input_key_to_just_pressed[InputKey::ASTERISK] = input_state.is_just_pressed(EKey::ASTERISK);
    iks.input_key_to_just_pressed[InputKey::LEFT_PARENTHESIS] = input_state.is_just_pressed(EKey::LEFT_PARENTHESIS);
    iks.input_key_to_just_pressed[InputKey::RIGHT_PARENTHESIS] = input_state.is_just_pressed(EKey::RIGHT_PARENTHESIS);
    iks.input_key_to_just_pressed[InputKey::UNDERSCORE] = input_state.is_just_pressed(EKey::UNDERSCORE);
    iks.input_key_to_just_pressed[InputKey::PLUS] = input_state.is_just_pressed(EKey::PLUS);

    iks.input_key_to_just_pressed[InputKey::LEFT_SQUARE_BRACKET] =
        input_state.is_just_pressed(EKey::LEFT_SQUARE_BRACKET);
    iks.input_key_to_just_pressed[InputKey::RIGHT_SQUARE_BRACKET] =
        input_state.is_just_pressed(EKey::RIGHT_SQUARE_BRACKET);
    iks.input_key_to_just_pressed[InputKey::LEFT_CURLY_BRACKET] = input_state.is_just_pressed(EKey::LEFT_CURLY_BRACKET);
    iks.input_key_to_just_pressed[InputKey::RIGHT_CURLY_BRACKET] =
        input_state.is_just_pressed(EKey::RIGHT_CURLY_BRACKET);

    iks.input_key_to_just_pressed[InputKey::COMMA] = input_state.is_just_pressed(EKey::COMMA);
    iks.input_key_to_just_pressed[InputKey::PERIOD] = input_state.is_just_pressed(EKey::PERIOD);
    iks.input_key_to_just_pressed[InputKey::LESS_THAN] = input_state.is_just_pressed(EKey::LESS_THAN);
    iks.input_key_to_just_pressed[InputKey::GREATER_THAN] = input_state.is_just_pressed(EKey::GREATER_THAN);

    iks.input_key_to_just_pressed[InputKey::CAPS_LOCK] = input_state.is_just_pressed(EKey::CAPS_LOCK);
    iks.input_key_to_just_pressed[InputKey::ESCAPE] = input_state.is_just_pressed(EKey::ESCAPE);
    iks.input_key_to_just_pressed[InputKey::ENTER] = input_state.is_just_pressed(EKey::ENTER);
    iks.input_key_to_just_pressed[InputKey::TAB] = input_state.is_just_pressed(EKey::TAB);
    iks.input_key_to_just_pressed[InputKey::BACKSPACE] = input_state.is_just_pressed(EKey::BACKSPACE);
    iks.input_key_to_just_pressed[InputKey::INSERT] = input_state.is_just_pressed(EKey::INSERT);
    iks.input_key_to_just_pressed[InputKey::DELETE] = input_state.is_just_pressed(EKey::DELETE);

    iks.input_key_to_just_pressed[InputKey::RIGHT] = input_state.is_just_pressed(EKey::RIGHT);
    iks.input_key_to_just_pressed[InputKey::LEFT] = input_state.is_just_pressed(EKey::LEFT);
    iks.input_key_to_just_pressed[InputKey::UP] = input_state.is_just_pressed(EKey::UP);
    iks.input_key_to_just_pressed[InputKey::DOWN] = input_state.is_just_pressed(EKey::DOWN);

    iks.input_key_to_just_pressed[InputKey::SLASH] = input_state.is_just_pressed(EKey::SLASH);
    iks.input_key_to_just_pressed[InputKey::QUESTION_MARK] = input_state.is_just_pressed(EKey::QUESTION_MARK);
    iks.input_key_to_just_pressed[InputKey::BACKSLASH] = input_state.is_just_pressed(EKey::BACKSLASH);
    iks.input_key_to_just_pressed[InputKey::PIPE] = input_state.is_just_pressed(EKey::PIPE);
    iks.input_key_to_just_pressed[InputKey::COLON] = input_state.is_just_pressed(EKey::COLON);
    iks.input_key_to_just_pressed[InputKey::SEMICOLON] = input_state.is_just_pressed(EKey::SEMICOLON);
    iks.input_key_to_just_pressed[InputKey::SINGLE_QUOTE] = input_state.is_just_pressed(EKey::SINGLE_QUOTE);
    iks.input_key_to_just_pressed[InputKey::DOUBLE_QUOTE] = input_state.is_just_pressed(EKey::DOUBLE_QUOTE);

    iks.input_key_to_just_pressed[InputKey::LEFT_SHIFT] = input_state.is_just_pressed(EKey::LEFT_SHIFT);
    iks.input_key_to_just_pressed[InputKey::RIGHT_SHIFT] = input_state.is_just_pressed(EKey::RIGHT_SHIFT);
    iks.input_key_to_just_pressed[InputKey::LEFT_CONTROL] = input_state.is_just_pressed(EKey::LEFT_CONTROL);
    iks.input_key_to_just_pressed[InputKey::RIGHT_CONTROL] = input_state.is_just_pressed(EKey::RIGHT_CONTROL);
    iks.input_key_to_just_pressed[InputKey::LEFT_ALT] = input_state.is_just_pressed(EKey::LEFT_ALT);
    iks.input_key_to_just_pressed[InputKey::RIGHT_ALT] = input_state.is_just_pressed(EKey::RIGHT_ALT);
    iks.input_key_to_just_pressed[InputKey::LEFT_SUPER] = input_state.is_just_pressed(EKey::LEFT_SUPER);
    iks.input_key_to_just_pressed[InputKey::RIGHT_SUPER] = input_state.is_just_pressed(EKey::RIGHT_SUPER);

    iks.input_key_to_just_pressed[InputKey::FUNCTION_KEY] = input_state.is_just_pressed(EKey::FUNCTION_KEY);
    iks.input_key_to_just_pressed[InputKey::MENU_KEY] = input_state.is_just_pressed(EKey::MENU_KEY);

    iks.input_key_to_just_pressed[InputKey::LEFT_MOUSE_BUTTON] = input_state.is_just_pressed(EKey::LEFT_MOUSE_BUTTON);
    iks.input_key_to_just_pressed[InputKey::RIGHT_MOUSE_BUTTON] = input_state.is_just_pressed(EKey::RIGHT_MOUSE_BUTTON);
    iks.input_key_to_just_pressed[InputKey::MIDDLE_MOUSE_BUTTON] =
        input_state.is_just_pressed(EKey::MIDDLE_MOUSE_BUTTON);
    iks.input_key_to_just_pressed[InputKey::SCROLL_UP] = input_state.is_just_pressed(EKey::SCROLL_UP);
    // NOTE: the following one is causing out of range, instead of bug fixing this, I'm just not going to because we're
    // probably not going to use scroll for a while and fix the bug later
    // iks.input_key_to_is_pressed[InputKey::SCROLL_DOWN] = input_state.is_pressed(EKey::SCROLL_DOWN);

    // not required
    // iks.input_key_to_is_pressed[InputKey::DUMMY] = input_state.is_pressed(EKey::DUMMY);

    return iks;
}

// stop doing this probably
using namespace ftxui;

struct EventHasher {
    std::size_t operator()(const Event &event) const { return std::hash<std::string>()(event.input()); }
};

// We use a function that returns a reference to a static local map
// instead of defining the map as a global/static variable directly.
// This avoids the static initialization order problem, where Event::a
// might not be fully initialized at the time the map is constructed.
// By using a static local variable, we ensure that Event::a is fully
// initialized before it's used as a key in the map.
const std::unordered_map<Event, std::vector<InputKey>, EventHasher> &get_event_to_input_keys() {
    static const std::unordered_map<Event, std::vector<InputKey>, EventHasher> event_to_input_keys = {
        // --- A ---
        {Event::a, {InputKey::a}},
        {Event::A, {InputKey::LEFT_SHIFT, InputKey::a}},
        {Event::CtrlA, {InputKey::LEFT_CONTROL, InputKey::a}},
        {Event::AltA, {InputKey::LEFT_ALT, InputKey::a}},
        {Event::CtrlAltA, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::a}},

        // --- B ---
        {Event::b, {InputKey::b}},
        {Event::B, {InputKey::LEFT_SHIFT, InputKey::b}},
        {Event::CtrlB, {InputKey::LEFT_CONTROL, InputKey::b}},
        {Event::AltB, {InputKey::LEFT_ALT, InputKey::b}},
        {Event::CtrlAltB, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::b}},

        // --- C ---
        {Event::c, {InputKey::c}},
        {Event::C, {InputKey::LEFT_SHIFT, InputKey::c}},
        {Event::CtrlC, {InputKey::LEFT_CONTROL, InputKey::c}},
        {Event::AltC, {InputKey::LEFT_ALT, InputKey::c}},
        {Event::CtrlAltC, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::c}},

        // --- D to Z ---
        {Event::d, {InputKey::d}},
        {Event::D, {InputKey::LEFT_SHIFT, InputKey::d}},
        {Event::CtrlD, {InputKey::LEFT_CONTROL, InputKey::d}},
        {Event::AltD, {InputKey::LEFT_ALT, InputKey::d}},
        {Event::CtrlAltD, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::d}},

        {Event::e, {InputKey::e}},
        {Event::E, {InputKey::LEFT_SHIFT, InputKey::e}},
        {Event::CtrlE, {InputKey::LEFT_CONTROL, InputKey::e}},
        {Event::AltE, {InputKey::LEFT_ALT, InputKey::e}},
        {Event::CtrlAltE, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::e}},

        {Event::f, {InputKey::f}},
        {Event::F, {InputKey::LEFT_SHIFT, InputKey::f}},
        {Event::CtrlF, {InputKey::LEFT_CONTROL, InputKey::f}},
        {Event::AltF, {InputKey::LEFT_ALT, InputKey::f}},
        {Event::CtrlAltF, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::f}},

        {Event::g, {InputKey::g}},
        {Event::G, {InputKey::LEFT_SHIFT, InputKey::g}},
        {Event::CtrlG, {InputKey::LEFT_CONTROL, InputKey::g}},
        {Event::AltG, {InputKey::LEFT_ALT, InputKey::g}},
        {Event::CtrlAltG, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::g}},

        {Event::h, {InputKey::h}},
        {Event::H, {InputKey::LEFT_SHIFT, InputKey::h}},
        {Event::CtrlH, {InputKey::LEFT_CONTROL, InputKey::h}},
        {Event::AltH, {InputKey::LEFT_ALT, InputKey::h}},
        {Event::CtrlAltH, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::h}},

        {Event::i, {InputKey::i}},
        {Event::I, {InputKey::LEFT_SHIFT, InputKey::i}},
        {Event::CtrlI, {InputKey::LEFT_CONTROL, InputKey::i}},
        {Event::AltI, {InputKey::LEFT_ALT, InputKey::i}},
        {Event::CtrlAltI, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::i}},

        {Event::j, {InputKey::j}},
        {Event::J, {InputKey::LEFT_SHIFT, InputKey::j}},
        {Event::CtrlJ, {InputKey::LEFT_CONTROL, InputKey::j}},
        {Event::AltJ, {InputKey::LEFT_ALT, InputKey::j}},
        {Event::CtrlAltJ, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::j}},

        {Event::k, {InputKey::k}},
        {Event::K, {InputKey::LEFT_SHIFT, InputKey::k}},
        {Event::CtrlK, {InputKey::LEFT_CONTROL, InputKey::k}},
        {Event::AltK, {InputKey::LEFT_ALT, InputKey::k}},
        {Event::CtrlAltK, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::k}},

        {Event::l, {InputKey::l}},
        {Event::L, {InputKey::LEFT_SHIFT, InputKey::l}},
        {Event::CtrlL, {InputKey::LEFT_CONTROL, InputKey::l}},
        {Event::AltL, {InputKey::LEFT_ALT, InputKey::l}},
        {Event::CtrlAltL, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::l}},

        {Event::m, {InputKey::m}},
        {Event::M, {InputKey::LEFT_SHIFT, InputKey::m}},
        {Event::CtrlM, {InputKey::LEFT_CONTROL, InputKey::m}},
        {Event::AltM, {InputKey::LEFT_ALT, InputKey::m}},
        {Event::CtrlAltM, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::m}},

        {Event::n, {InputKey::n}},
        {Event::N, {InputKey::LEFT_SHIFT, InputKey::n}},
        {Event::CtrlN, {InputKey::LEFT_CONTROL, InputKey::n}},
        {Event::AltN, {InputKey::LEFT_ALT, InputKey::n}},
        {Event::CtrlAltN, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::n}},

        {Event::o, {InputKey::o}},
        {Event::O, {InputKey::LEFT_SHIFT, InputKey::o}},
        {Event::CtrlO, {InputKey::LEFT_CONTROL, InputKey::o}},
        {Event::AltO, {InputKey::LEFT_ALT, InputKey::o}},
        {Event::CtrlAltO, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::o}},

        {Event::p, {InputKey::p}},
        {Event::P, {InputKey::LEFT_SHIFT, InputKey::p}},
        {Event::CtrlP, {InputKey::LEFT_CONTROL, InputKey::p}},
        {Event::AltP, {InputKey::LEFT_ALT, InputKey::p}},
        {Event::CtrlAltP, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::p}},

        {Event::q, {InputKey::q}},
        {Event::Q, {InputKey::LEFT_SHIFT, InputKey::q}},
        {Event::CtrlQ, {InputKey::LEFT_CONTROL, InputKey::q}},
        {Event::AltQ, {InputKey::LEFT_ALT, InputKey::q}},
        {Event::CtrlAltQ, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::q}},

        {Event::r, {InputKey::r}},
        {Event::R, {InputKey::LEFT_SHIFT, InputKey::r}},
        {Event::CtrlR, {InputKey::LEFT_CONTROL, InputKey::r}},
        {Event::AltR, {InputKey::LEFT_ALT, InputKey::r}},
        {Event::CtrlAltR, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::r}},

        {Event::s, {InputKey::s}},
        {Event::S, {InputKey::LEFT_SHIFT, InputKey::s}},
        {Event::CtrlS, {InputKey::LEFT_CONTROL, InputKey::s}},
        {Event::AltS, {InputKey::LEFT_ALT, InputKey::s}},
        {Event::CtrlAltS, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::s}},

        {Event::t, {InputKey::t}},
        {Event::T, {InputKey::LEFT_SHIFT, InputKey::t}},
        {Event::CtrlT, {InputKey::LEFT_CONTROL, InputKey::t}},
        {Event::AltT, {InputKey::LEFT_ALT, InputKey::t}},
        {Event::CtrlAltT, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::t}},

        {Event::u, {InputKey::u}},
        {Event::U, {InputKey::LEFT_SHIFT, InputKey::u}},
        {Event::CtrlU, {InputKey::LEFT_CONTROL, InputKey::u}},
        {Event::AltU, {InputKey::LEFT_ALT, InputKey::u}},
        {Event::CtrlAltU, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::u}},

        {Event::v, {InputKey::v}},
        {Event::V, {InputKey::LEFT_SHIFT, InputKey::v}},
        {Event::CtrlV, {InputKey::LEFT_CONTROL, InputKey::v}},
        {Event::AltV, {InputKey::LEFT_ALT, InputKey::v}},
        {Event::CtrlAltV, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::v}},

        {Event::w, {InputKey::w}},
        {Event::W, {InputKey::LEFT_SHIFT, InputKey::w}},
        {Event::CtrlW, {InputKey::LEFT_CONTROL, InputKey::w}},
        {Event::AltW, {InputKey::LEFT_ALT, InputKey::w}},
        {Event::CtrlAltW, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::w}},

        {Event::x, {InputKey::x}},
        {Event::X, {InputKey::LEFT_SHIFT, InputKey::x}},
        {Event::CtrlX, {InputKey::LEFT_CONTROL, InputKey::x}},
        {Event::AltX, {InputKey::LEFT_ALT, InputKey::x}},
        {Event::CtrlAltX, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::x}},

        {Event::y, {InputKey::y}},
        {Event::Y, {InputKey::LEFT_SHIFT, InputKey::y}},
        {Event::CtrlY, {InputKey::LEFT_CONTROL, InputKey::y}},
        {Event::AltY, {InputKey::LEFT_ALT, InputKey::y}},
        {Event::CtrlAltY, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::y}},

        {Event::z, {InputKey::z}},
        {Event::Z, {InputKey::LEFT_SHIFT, InputKey::z}},
        {Event::CtrlZ, {InputKey::LEFT_CONTROL, InputKey::z}},
        {Event::AltZ, {InputKey::LEFT_ALT, InputKey::z}},
        {Event::CtrlAltZ, {InputKey::LEFT_CONTROL, InputKey::LEFT_ALT, InputKey::z}},

        // --- Arrow keys ---
        {Event::ArrowLeft, {InputKey::LEFT}},
        {Event::ArrowLeftCtrl, {InputKey::LEFT_CONTROL, InputKey::LEFT}},
        {Event::ArrowRight, {InputKey::RIGHT}},
        {Event::ArrowRightCtrl, {InputKey::LEFT_CONTROL, InputKey::RIGHT}},
        {Event::ArrowUp, {InputKey::UP}},
        {Event::ArrowUpCtrl, {InputKey::LEFT_CONTROL, InputKey::UP}},
        {Event::ArrowDown, {InputKey::DOWN}},
        {Event::ArrowDownCtrl, {InputKey::LEFT_CONTROL, InputKey::DOWN}},

        {Event::Character(" "), {InputKey::SPACE}},
        {Event::Character(";"), {InputKey::SEMICOLON}},
        {Event::Character("/"), {InputKey::SLASH}},
        {Event::Character("?"), {InputKey::LEFT_SHIFT, InputKey::SLASH}},
        {Event::Character(":"), {InputKey::LEFT_SHIFT, InputKey::SEMICOLON}},

        {Event::Character("0"), {InputKey::ZERO}},
        {Event::Character("1"), {InputKey::ONE}},
        {Event::Character("2"), {InputKey::TWO}},
        {Event::Character("3"), {InputKey::THREE}},
        {Event::Character("4"), {InputKey::FOUR}},
        {Event::Character("5"), {InputKey::FIVE}},
        {Event::Character("6"), {InputKey::SIX}},
        {Event::Character("7"), {InputKey::SEVEN}},
        {Event::Character("8"), {InputKey::EIGHT}},
        {Event::Character("9"), {InputKey::NINE}},

        {Event::Character("="), {InputKey::EQUAL}},
        {Event::Character("+"), {InputKey::LEFT_SHIFT, InputKey::EQUAL}},

        {Event::Character("'"), {InputKey::SINGLE_QUOTE}},
        {Event::Character("\""), {InputKey::LEFT_SHIFT, InputKey::SINGLE_QUOTE}},

        {Event::Character(","), {InputKey::COMMA}},
        {Event::Character("<"), {InputKey::LEFT_SHIFT, InputKey::COMMA}},
        {Event::Character("."), {InputKey::PERIOD}},
        {Event::Character(">"), {InputKey::LEFT_SHIFT, InputKey::PERIOD}},

        {Event::Character("!"), {InputKey::LEFT_SHIFT, InputKey::ONE}},
        {Event::Character("@"), {InputKey::LEFT_SHIFT, InputKey::TWO}},
        {Event::Character("#"), {InputKey::LEFT_SHIFT, InputKey::THREE}},
        {Event::Character("$"), {InputKey::LEFT_SHIFT, InputKey::FOUR}},
        {Event::Character("%"), {InputKey::LEFT_SHIFT, InputKey::FIVE}},
        {Event::Character("^"), {InputKey::LEFT_SHIFT, InputKey::SIX}},
        {Event::Character("&"), {InputKey::LEFT_SHIFT, InputKey::SEVEN}},
        {Event::Character("*"), {InputKey::LEFT_SHIFT, InputKey::EIGHT}},

        {Event::Character("("), {InputKey::LEFT_SHIFT, InputKey::NINE}},
        {Event::Character(")"), {InputKey::LEFT_SHIFT, InputKey::ZERO}},

        {Event::Character("["), {InputKey::LEFT_SQUARE_BRACKET}},
        {Event::Character("]"), {InputKey::RIGHT_SQUARE_BRACKET}},

        {Event::Character("{"), {InputKey::LEFT_SHIFT, InputKey::LEFT_SQUARE_BRACKET}},
        {Event::Character("}"), {InputKey::LEFT_SHIFT, InputKey::RIGHT_SQUARE_BRACKET}},

        // --- Miscellaneous keys ---
        {Event::Escape, {InputKey::ESCAPE}},
        {Event::Return, {InputKey::ENTER}},
        {Event::Tab, {InputKey::TAB}},
        {Event::TabReverse, {InputKey::LEFT_SHIFT, InputKey::TAB}},
        {Event::Backspace, {InputKey::BACKSPACE}},
        {Event::Delete, {InputKey::DELETE}},
        {Event::Insert, {InputKey::INSERT}},

        // --- Navigation keys (replace DUMMY with correct keys) ---
        // {Event::Home, {InputKey::HOME}},
        // {Event::End, {InputKey::END}},
        // {Event::PageUp, {InputKey::PAGE_UP}},
        // {Event::PageDown, {InputKey::PAGE_DOWN}},
        //
        // // --- Function keys ---
        // {Event::F1, {InputKey::F1}},
        // {Event::F2, {InputKey::F2}},
        // {Event::F3, {InputKey::F3}},
        // {Event::F4, {InputKey::F4}},
        // {Event::F5, {InputKey::F5}},
        // {Event::F6, {InputKey::F6}},
        // {Event::F7, {InputKey::F7}},
        // {Event::F8, {InputKey::F8}},
        // {Event::F9, {InputKey::F9}},
        // {Event::F10, {InputKey::F10}},
        // {Event::F11, {InputKey::F11}},
        // {Event::F12, {InputKey::F12}},
    };
    return event_to_input_keys;
}

class FileLogger {
  public:
    FileLogger(const std::string &filename) {
        // Open and truncate the file on start
        file_.open(filename, std::ios::out | std::ios::trunc);
    }

    ~FileLogger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    template <typename T> FileLogger &operator<<(const T &value) {
        file_ << value;
        file_.flush(); // Optional: flush immediately
        return *this;
    }

    // Support std::endl and other manipulators
    FileLogger &operator<<(std::ostream &(*manip)(std::ostream &)) {
        file_ << manip;
        file_.flush();
        return *this;
    }

  private:
    std::ofstream file_;
};

Element generate_status_bar(ModalEditor &modal_editor, const std::string &filename) {
    // Status Bar
    std::string mode_str;

    switch (modal_editor.current_mode) {
    case MOVE_AND_EDIT:
        mode_str = " -- NORMAL -- ";
        break;
    case INSERT:
        mode_str = " -- INSERT -- ";
        break;
    case VISUAL_SELECT:
        mode_str = " -- VISUAL SELECT -- ";
        break;
    case COMMAND:
        mode_str = " -- COMMAND -- ";
        break;
    }

    // std::string save_str = show_saved_msg ? "✅ Saved" : "";
    std::string save_str = "save placeholder";
    // Clock Stuff v
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_time_t);
    int time_hours = now_tm->tm_hour;
    int time_minutes = now_tm->tm_min;
    int time_seconds = now_tm->tm_sec;
    std::string seconds_buffer;
    std::string minutes_buffer;
    if (time_seconds < 10)
        seconds_buffer = ":0";
    else
        seconds_buffer = ":";
    if (time_minutes < 10)
        minutes_buffer = ":0";
    else
        minutes_buffer = ":";
    // Clock Stuff ^
    //
    unsigned int num_lines_in_current_file = modal_editor.viewport.buffer->line_count();

    int percentage_vertical_scroll =
        (modal_editor.viewport.active_buffer_line_under_cursor + 1) * 100 / num_lines_in_current_file;
    // int percentage_horizontal_scroll = (modal_editor.viewport.active_buffer_col_under_cursor + 1) * 100 /
    // (lines[cursor_row].size() + 1);
    int percentage_horizontal_scroll = 0;
    auto status = hbox({
        text(mode_str) | color(Color::Cyan),
        text("  "),
        text(save_str) | color(Color::Green),
        text(" "),
        text(std::to_string(percentage_vertical_scroll)) | color(Color::Cyan),
        text("% "),
        text(std::to_string(modal_editor.viewport.active_buffer_line_under_cursor + 1)) | color(Color::Cyan),
        text("/"),
        text(std::to_string(num_lines_in_current_file)) | color(Color::Green),
        text(" - "),
        text(std::to_string(percentage_horizontal_scroll)) | color(Color::Cyan),
        text("% "),
        text(std::to_string(modal_editor.viewport.active_buffer_col_under_cursor + 1)) | color(Color::Cyan),
        // text("/"), text(std::to_string(lines[cursor_row].size() + 1)) | color(Color::Green),
        filler(),
        text(std::to_string(time_hours)),
        text(minutes_buffer),
        text(std::to_string(time_minutes)),
        text(seconds_buffer),
        text(std::to_string(time_seconds)),
        text(" "),
        text(filename),
    });
    return status;
}

Component create_fuzzy_file_selection_modal(std::function<void()> do_nothing, std::function<void()> hide_modal,
                                            ModalEditor &modal_editor) {

    // TODO: remove
    auto component = Container::Vertical({});

    // Polish how the two buttons are rendered:

    component |= Renderer([&](Element inner) {
        std::vector<Element> matched_file_texts;
        unsigned int iter = 0;
        for (const auto &matched_file : modal_editor.fuzzy_file_selection_modal.currently_matched_results) {
            auto t = text(matched_file);
            if (iter == modal_editor.fuzzy_file_selection_modal.current_selection_index) {
                t |= bgcolor(Color::Grey63);
            }
            matched_file_texts.push_back(t);
            ++iter;
        }

        std::reverse(matched_file_texts.begin(), matched_file_texts.end());

        std::vector<Element> x = {
            text("Modal component "),
            separator(),
            inner,
        };
        return vbox({
                   window(text("results"), vbox(matched_file_texts)),
                   window(text("search"), text(modal_editor.fuzzy_file_selection_modal.search_query)),
               }) |
               size(WIDTH, GREATER_THAN, 50) // TODO: remove this hard value instead use percentages of active viewport
               | size(HEIGHT, GREATER_THAN, 25) //
               | border;                        //
    });
    return component;
}

Component create_open_buffers_selection_modal(std::function<void()> do_nothing, std::function<void()> hide_modal,
                                              ModalEditor &modal_editor) {

    // TODO: remove
    auto component = Container::Vertical({});

    // Polish how the two buttons are rendered:

    component |= Renderer([&](Element inner) {
        std::vector<Element> matched_file_texts;
        unsigned int iter = 0;
        for (const auto &active_file_buffer : modal_editor.viewport.active_file_buffers) {
            auto t = text(active_file_buffer->current_file_path);
            if (iter == modal_editor.fuzzy_file_selection_modal.current_selection_index) {
                t |= bgcolor(Color::Grey63);
            }
            matched_file_texts.push_back(t);
            ++iter;
        }

        std::reverse(matched_file_texts.begin(), matched_file_texts.end());

        std::vector<Element> x = {
            text("Modal component "),
            separator(),
            inner,
        };
        return vbox({
                   window(text("results"), vbox(matched_file_texts)),
                   window(text("search"), text(modal_editor.fuzzy_file_selection_modal.search_query)),
               }) |
               size(WIDTH, GREATER_THAN, 50) // TODO: remove this hard value instead use percentages of active viewport
               | size(HEIGHT, GREATER_THAN, 25) //
               | border;                        //
    });
    return component;
}

int main(int argc, char *argv[]) {

    // NOTE: because the lsp server runs in another thread asychronously
    // if we allow the callbacks to be run the moment the lsp server responds
    // then the order in which logic runs can be different on each iteration
    // thus we queue up the callback logic into a vector so that we can
    // handle it in the main thread
    std::vector<std::function<void()>> lsp_callbacks_to_run_synchronously;

    ResourcePath rp(false);

#if defined(_WIN32) || defined(_WIN64)
    LSPClient lsp_client(
        rp.gfp("C:\\Users\\ccn\\projects\\cpp-toolbox-organization\\editor\\").string(), "cpp",
        rp.gfp("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\clangd.exe")
            .string());
#else
    // LSPClient lsp_client("/home/ccn/projects/cpp-toolbox-organization/editor/");
    LSPClient lsp_client("");
#endif

    // std::thread thread([&] {
    //     while (true) {
    //         lsp_client.process_requests_and_responses();
    //     }
    // });

    // auto screen = ScreenInteractive::TerminalOutput();
    auto screen = ScreenInteractive::Fullscreen();

    FileLogger fl("logs.txt");

    fl << "screen dimx: " << screen.dimx() << " screen dimy: " << screen.dimy() << std::endl;

    int saved_for_automatic_column_adjustment = 0;
    int saved_last_col_for_automatic_column_adjustment = 0;
    int saved_last_line_for_automatic_column_adjustment = 0;

    unsigned int windowed_screen_width_px = 700;
    unsigned int windowed_screen_height_px = 700;

    bool automatic_column_adjustment = false;
    std::string username = "tbx_user";

    // numbers must be odd to have a center
    int num_lines = 41;
    int num_cols = 121;
    int center_line = num_lines / 2;
    int center_col = num_cols / 2;

    bool start_in_fullscreen = false;

    std::filesystem::path config_path = "~/.tbx_cfg.ini";
    // Configuration config(config_path, section_key_to_config_logic);

    std::cout << username << std::endl;

    PeriodicSignal one_second_signal_for_status_bar_time_update(2);

    std::cout << get_executable_path(argv) << std::endl;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("mwe_shader_cache_logs.txt", true);
    file_sink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};

    std::string search_dir = ".";
    std::vector<std::string> ignore_dirs = {"build", ".git", "__pycache__"};
    std::vector<std::filesystem::path> searchable_files = rec_get_all_files(search_dir, ignore_dirs);
    /*for (const auto &file : searchable_files) {*/
    /*    std::cout << file.string() << '\n';*/
    /*}*/

    // FS BROWSER UI START
    std::vector<int> doids_for_textboxes_for_active_directory_for_later_removal;

    // InputState input_state;
    std::cout << "after constructor" << std::endl;

    int line_where_selection_mode_started = -1;
    int col_where_selection_mode_started = -1;

    int center_idx_x = num_cols / 2;
    int center_idx_y = num_lines / 2;

    glm::vec2 active_visible_screen_position;

    // Check if the user provided a file argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1; // Return an error code if no argument is provided
    }

    std::string filename = argv[1];

    // initialize the system.
    auto file_buffer = std::make_shared<LineTextBuffer>();

    Viewport viewport(file_buffer, num_lines, num_cols, center_idx_y, center_idx_x);

    ModalEditor modal_editor(viewport);
    modal_editor.switch_files(filename, true);

    InputKeyState input_key_state = InputKeyState();

    fl << "initial iks size: " << input_key_state.input_key_to_is_pressed.size() << std::endl;

    auto event_to_input_keys = get_event_to_input_keys();

    auto it = event_to_input_keys.find(Event::a);
    if (it != event_to_input_keys.end()) {
        fl << "we have 'a'" << std::endl;
    } else {
        fl << "we do not have 'a'" << std::endl;
    }

    fl << "lookup input: " << Event::a.input() << std::endl;
    fl << "key input: " << event_to_input_keys.begin()->first.input() << std::endl;

    // TODO: Terrible, I don't know why the above reports back a size 2, which makes no sense
    // therefore I just "fix" it here, something is really wrong with get_event_to_input_keys function.
    event_to_input_keys[Event::Return] = {InputKey::ENTER};

    it = event_to_input_keys.find(Event::Return);
    if (it != event_to_input_keys.end()) {
        fl << "Found mapping for Return key" << std::endl;
        fl << "Number of keys in mapping: " << it->second.size() << std::endl;

        if (it->second.size() == 1 && it->second[0] == InputKey::ENTER) {
            fl << "Return is correctly mapped to exactly one key: InputKey::ENTER" << std::endl;
        } else {
            fl << "Unexpected mapping for Return key:" << std::endl;
            for (const auto &key : it->second) {
                fl << "  - Key: " << input_key_to_string(key, true) << std::endl;
            }
        }
    } else {
        fl << "No mapping found for Return key" << std::endl;
    }

    // auto c = Canvas(num_cols, num_lines);
    // c.DrawPointCircle(5, 5, 5);

    std::thread animation_thread([&] {
        while (not modal_editor.requested_quit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ~60 FPS
            screen.PostEvent(Event::Custom);
        }
        screen.Exit(); // triggers exit from screen.Loop
    });

    std::vector<Event> keys;

    auto component = Container::Vertical({
        Renderer([&] {
            num_lines = screen.dimy() - 2 * 4; // space for status bar
            num_cols = screen.dimx();

            modal_editor.viewport.num_cols = num_cols;
            modal_editor.viewport.num_lines = num_lines;

            modal_editor.viewport.half_num_cols = num_cols / 2;
            modal_editor.viewport.half_num_lines = num_lines / 2;

            center_line = num_lines / 2;
            center_col = num_cols / 2;

            auto c = Canvas(num_cols * 2, num_lines * 4); // Match canvas size to your drawing area

            unsigned int vsel_min_buf_col = std::min(modal_editor.buffer_col_where_selection_mode_started,
                                                     modal_editor.viewport.active_buffer_col_under_cursor);
            unsigned int vsel_max_buf_col = std::max(modal_editor.buffer_col_where_selection_mode_started,
                                                     modal_editor.viewport.active_buffer_col_under_cursor);
            unsigned int vsel_min_buf_line = std::min(modal_editor.buffer_line_where_selection_mode_started,
                                                      modal_editor.viewport.active_buffer_line_under_cursor);
            unsigned int vsel_max_buf_line = std::max(modal_editor.buffer_line_where_selection_mode_started,
                                                      modal_editor.viewport.active_buffer_line_under_cursor);

            for (int vp_line = 0; vp_line < num_lines; vp_line++) {
                for (int vp_col = 0; vp_col < num_cols; vp_col++) {

                    bool line_in_visual_selection = false;
                    bool col_in_visual_selection = false;
                    bool cell_should_be_selected = false;

                    if (modal_editor.current_mode == VISUAL_SELECT) {
                        auto [buf_line, buf_col] =
                            modal_editor.viewport.viewport_idx_to_centered_buffer_idx(vp_line, vp_col);
                        line_in_visual_selection = vsel_min_buf_line <= buf_line and buf_line <= vsel_max_buf_line;
                        col_in_visual_selection = vsel_min_buf_col <= buf_col and buf_col <= vsel_max_buf_col;
                        cell_should_be_selected = line_in_visual_selection and col_in_visual_selection;
                    }

                    bool position_is_within_selection_column_wise =
                        modal_editor.buffer_col_where_selection_mode_started <= vp_line and
                        vp_line <= modal_editor.viewport.active_buffer_col_under_cursor;

                    std::string cell_char(1, viewport.get_symbol_at(vp_line, vp_col));
                    c.DrawText(vp_col * 2, vp_line * 4, cell_char, [&](Pixel &p) {
                        bool at_center = vp_line == center_line and vp_col == center_col;
                        // the modal editor will always cover up the thing so in that case don't draw it
                        if (at_center and not modal_editor.fuzzy_file_selection_modal.active) {
                            p.background_color = Color::White;
                            p.foreground_color = Color::Black;
                        } else if (cell_should_be_selected) {
                            p.background_color = Color::Grey63;
                        } else {
                            p.foreground_color = Color::White;
                        }
                    });
                }
            }

            if (modal_editor.fuzzy_file_selection_modal.active) {
            }

            auto just_pressed_keys = input_key_state.get_keys_just_pressed_this_tick();
            fl << "in Renderer Just pressed keys this tick:";
            for (const auto &key_str : just_pressed_keys) {
                fl << " " << key_str;
            }
            fl << std::endl;

            input_key_state.process_input_state();

            auto command_and_update_bar = text(modal_editor.command_bar_input);

            auto status = generate_status_bar(modal_editor, modal_editor.viewport.buffer->current_file_path);

            return vbox(canvas(std::move(c)) | border, status, command_and_update_bar);
        }),
    });

    auto hide_modal = [&] { modal_editor.fuzzy_file_selection_modal.active = false; };
    auto do_nothing = [&] {};

    auto modal_component = create_fuzzy_file_selection_modal(do_nothing, hide_modal, modal_editor);

    component |= Modal(modal_component, &modal_editor.fuzzy_file_selection_modal.active);

    component |= CatchEvent([&](Event event) {
        keys.push_back(event);

        fl << "got event" << std::endl;

        if (event == Event::Return) {
            fl << "got return" << std::endl;
        }

        auto it = event_to_input_keys.find(event);
        if (it != event_to_input_keys.end()) {
            for (const auto &key : it->second) {
                auto str = input_key_to_string(key, false);
                fl << "got str: " << str << std::endl;
                input_key_state.input_key_to_is_pressed[key] = true;
                bool was_pressed = input_key_state.input_key_to_is_pressed_prev[key];
                input_key_state.input_key_to_just_pressed[key] =
                    input_key_state.input_key_to_is_pressed[key] && !was_pressed;
                fl << "in catch, is pressed: " << input_key_state.input_key_to_is_pressed[key]
                   << " was not pressed: " << !was_pressed << std::endl;
            }
            fl << std::endl;
        } else {
            fl << "No mapping found for event: " << std::endl;
        }

        // render to the canvas

        // fl << "post event" << std::endl;
        // screen.PostEvent(Event::Custom);
        // screen.RequestAnimationFrame();

        modal_editor.iks = input_key_state;

        std::string search_dir = ".";
        std::vector<std::string> ignore_dirs = {"build", ".git", "__pycache__"};
        std::vector<std::filesystem::path> searchable_files = rec_get_all_files(search_dir, ignore_dirs);

        viewport.save_previous_viewport_screen();
        modal_editor.run_key_logic(searchable_files);

        auto keys = modal_editor.iks.get_keys_just_pressed_this_tick();

        if (std::find(keys.begin(), keys.end(), " ") != keys.end()) {
            fl << "GOT SPACE" << std::endl;
        }

        TemporalBinarySignal::process_all();
        return false;
    });

    screen.Loop(component);
    animation_thread.join();

    // thread.detach();

    exit(EXIT_SUCCESS);
}
