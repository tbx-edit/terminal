#ifndef MODAL_EDITOR_HPP
#define MODAL_EDITOR_HPP

#include <filesystem>

#include "../utility/hierarchical_history/hierarchical_history.hpp"
#include "../utility/temporal_binary_signal/temporal_binary_signal.hpp"
#include "../utility/text_diff/text_diff.hpp"
#include "../utility/regex_command_runner/regex_command_runner.hpp"
#include "../graphics/viewport/viewport.hpp"

// clang-format off
enum class InputKey {
    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,

    SPACE,
    GRAVE_ACCENT,
    TILDE,

    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, ZERO,
    MINUS, EQUAL,

    EXCLAMATION_POINT, AT_SIGN, NUMBER_SIGN, DOLLAR_SIGN, PERCENT_SIGN,
    CARET, AMPERSAND, ASTERISK, LEFT_PARENTHESIS, RIGHT_PARENTHESIS,
    UNDERSCORE, PLUS,

    LEFT_SQUARE_BRACKET, RIGHT_SQUARE_BRACKET,
    LEFT_CURLY_BRACKET, RIGHT_CURLY_BRACKET,

    COMMA, PERIOD, LESS_THAN, GREATER_THAN,

    CAPS_LOCK, ESCAPE, ENTER, TAB, BACKSPACE, INSERT, DELETE,

    RIGHT, LEFT, UP, DOWN,

    SLASH, QUESTION_MARK, BACKSLASH, PIPE,
    COLON, SEMICOLON, SINGLE_QUOTE, DOUBLE_QUOTE,

    LEFT_SHIFT, RIGHT_SHIFT,
    LEFT_CONTROL, RIGHT_CONTROL,
    LEFT_ALT, RIGHT_ALT,
    LEFT_SUPER, RIGHT_SUPER,

    FUNCTION_KEY, MENU_KEY,

    LEFT_MOUSE_BUTTON, RIGHT_MOUSE_BUTTON, MIDDLE_MOUSE_BUTTON,
    SCROLL_UP, SCROLL_DOWN,

    DUMMY
};

#include <array>

constexpr std::array<InputKey, static_cast<size_t>(InputKey::DUMMY)> all_input_keys = {
    InputKey::a, InputKey::b, InputKey::c, InputKey::d, InputKey::e, InputKey::f,
    InputKey::g, InputKey::h, InputKey::i, InputKey::j, InputKey::k, InputKey::l,
    InputKey::m, InputKey::n, InputKey::o, InputKey::p, InputKey::q, InputKey::r,
    InputKey::s, InputKey::t, InputKey::u, InputKey::v, InputKey::w, InputKey::x,
    InputKey::y, InputKey::z,

    InputKey::SPACE,
    InputKey::GRAVE_ACCENT,
    InputKey::TILDE,

    InputKey::ONE, InputKey::TWO, InputKey::THREE, InputKey::FOUR, InputKey::FIVE,
    InputKey::SIX, InputKey::SEVEN, InputKey::EIGHT, InputKey::NINE, InputKey::ZERO,
    InputKey::MINUS, InputKey::EQUAL,

    InputKey::EXCLAMATION_POINT, InputKey::AT_SIGN, InputKey::NUMBER_SIGN,
    InputKey::DOLLAR_SIGN, InputKey::PERCENT_SIGN, InputKey::CARET,
    InputKey::AMPERSAND, InputKey::ASTERISK, InputKey::LEFT_PARENTHESIS,
    InputKey::RIGHT_PARENTHESIS, InputKey::UNDERSCORE, InputKey::PLUS,

    InputKey::LEFT_SQUARE_BRACKET, InputKey::RIGHT_SQUARE_BRACKET,
    InputKey::LEFT_CURLY_BRACKET, InputKey::RIGHT_CURLY_BRACKET,

    InputKey::COMMA, InputKey::PERIOD, InputKey::LESS_THAN, InputKey::GREATER_THAN,

    InputKey::CAPS_LOCK, InputKey::ESCAPE, InputKey::ENTER, InputKey::TAB,
    InputKey::BACKSPACE, InputKey::INSERT, InputKey::DELETE,

    InputKey::RIGHT, InputKey::LEFT, InputKey::UP, InputKey::DOWN,

    InputKey::SLASH, InputKey::QUESTION_MARK, InputKey::BACKSLASH, InputKey::PIPE,
    InputKey::COLON, InputKey::SEMICOLON, InputKey::SINGLE_QUOTE, InputKey::DOUBLE_QUOTE,

    InputKey::LEFT_SHIFT, InputKey::RIGHT_SHIFT,
    InputKey::LEFT_CONTROL, InputKey::RIGHT_CONTROL,
    InputKey::LEFT_ALT, InputKey::RIGHT_ALT,
    InputKey::LEFT_SUPER, InputKey::RIGHT_SUPER,

    InputKey::FUNCTION_KEY, InputKey::MENU_KEY,

    InputKey::LEFT_MOUSE_BUTTON, InputKey::RIGHT_MOUSE_BUTTON,
    InputKey::MIDDLE_MOUSE_BUTTON, InputKey::SCROLL_UP, InputKey::SCROLL_DOWN
};

std::string input_key_to_string(InputKey key, bool shift_pressed);

// clang-format on

struct InputKeyState {

    std::unordered_map<InputKey, bool> input_key_to_is_pressed;
    std::unordered_map<InputKey, bool> input_key_to_just_pressed;
    std::unordered_map<InputKey, bool> input_key_to_is_pressed_prev;

    InputKeyState() {
        for (InputKey key : all_input_keys) {
            input_key_to_is_pressed[key] = false;
            input_key_to_just_pressed[key] = false;
            input_key_to_is_pressed_prev[key] = false;
        }
    }

    bool is_pressed(const InputKey &input_key) const { return input_key_to_is_pressed.at(input_key); }
    bool is_just_pressed(const InputKey &input_key) const { return input_key_to_just_pressed.at(input_key); }

    // NOTE: this doesn't make sense when more than one key is pressed this tick, I'm just going to make the bad
    // assumption that that will not really occur too often, if it does then we'll have to fix this
    std::vector<std::string> get_keys_just_pressed_this_tick() {
        std::vector<std::string> result;
        bool shift_pressed = is_pressed(InputKey::LEFT_SHIFT) || is_pressed(InputKey::RIGHT_SHIFT);

        for (const auto &[key, just_pressed] : input_key_to_just_pressed) {
            if (!just_pressed)
                continue;

            // Skip modifier keys
            switch (key) {
            case InputKey::ENTER:
            case InputKey::LEFT_SHIFT:
            case InputKey::RIGHT_SHIFT:
            case InputKey::LEFT_CONTROL:
            case InputKey::RIGHT_CONTROL:
            case InputKey::LEFT_ALT:
            case InputKey::RIGHT_ALT:
            case InputKey::LEFT_SUPER:
            case InputKey::RIGHT_SUPER:
                continue;
            default:
                break;
            }

            std::string str = input_key_to_string(key, shift_pressed);
            if (!str.empty())
                result.push_back(str);
        }

        return result;
    }

    void process_input_state() {
        input_key_to_is_pressed_prev = input_key_to_is_pressed;
        for (auto &[key, is_pressed] : input_key_to_is_pressed) {
            is_pressed = false;
        }

        for (auto &[key, is_pressed] : input_key_to_just_pressed) {
            is_pressed = false;
        }
    }
};

enum EditorMode {
    MOVE_AND_EDIT,
    INSERT,
    VISUAL_SELECT,
    COMMAND,
};

class ModalEditor {
  public:
    // main stuff start
    EditorMode current_mode = MOVE_AND_EDIT;
    std::string get_mode_string();
    TemporalBinarySignal mode_change_signal;
    Viewport &viewport;
    InputKeyState iks = InputKeyState();

    ModalEditor(Viewport &viewport);

    // main stuff end

    bool requested_quit = false;

    std::string command_bar_input;
    TemporalBinarySignal command_bar_input_signal;
    TemporalBinarySignal insert_mode_signal;

    int buffer_line_where_selection_mode_started = -1;
    int buffer_col_where_selection_mode_started = -1;

    // regex command runner [[
    RegexCommandRunner regex_command_runner;
    bool regex_command_runner_has_been_configured = false;
    std::string potential_regex_command = "";
    // regex command runner ]]

    // searching within file [[
    std::vector<TextRange> file_search_results;
    int current_file_search_index = 0; // to keep track of the current search result
    bool file_search_is_active = false;
    // searching within file ]]

    // fuzzy_file_selection_modal [[
    bool fuzzy_file_selection_modal_is_active = false;
    std::string fuzzy_file_selection_search_query = "";
    TemporalBinarySignal fuzzy_file_selection_search_results_changed_signal;
    std::vector<std::string> fuzzy_file_selection_currently_matched_files;
    unsigned int fuzzy_file_selection_idx = 0;
    unsigned int fuzzy_file_selection_max_matched_files = 10;
    // fuzzy_file_selection_modal ]]

    // active_file_buffers_modal [[
    bool active_file_buffers_modal_is_active = false;
    std::string active_file_buffers_modal_search_query = "";
    TemporalBinarySignal active_file_buffers_modal_search_results_changed_signal;
    std::vector<std::string> active_file_buffers_modal_currently_matched_files;
    // active_file_buffers_modal ]]

    // actual logic here
    void switch_files(const std::string &file_to_open, bool store_movements_to_history);
    void insert_character_in_insert_mode(unsigned int character_code);

    void delete_at_current_cursor_position_logic();
    void delete_line_current_cursor_position();
    void paste_at_cursor_position_logic(const std::smatch &m);
    void start_visual_selection();
    void enter_insert_mode();
    void enter_insert_mode_after_cursor_position();
    void enter_insert_mode_at_end_of_line();
    void enter_insert_mode_in_front_of_first_non_whitespace_character_on_active_line();
    void move_cursor_to_first_non_whitespace_character_on_active_line();
    void handle_hjkl_with_number_modifier(const std::smatch &m);
    void go_to_specific_line_number(const std::smatch &m);
    void change_or_delete_till_or_find_to_character(const std::smatch &m);
    void change_or_delete_using_word_motion(const std::smatch &m);
    void change_or_delete_inside_or_around_brackets(const std::smatch &m);
    void open_new_line_below_or_above_current_line(const std::smatch &m);
    void undo();
    void redo();
    void launch_search_files();
    void search_active_buffers();
    void switch_to_cpp_source_file();
    void switch_to_hpp_source_file();

    bool run_command_bar_command();
    bool run_non_regex_based_move_and_edit_commands();
    void run_key_logic(std::vector<std::filesystem::path> &searchable_files);
};

#endif // MODAL_EDITOR_HPP
