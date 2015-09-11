#ifndef TREE_SITTER_PARSER_H_
#define TREE_SITTER_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "tree_sitter/runtime.h"

#define ts_lex_state_error 0
#define TS_DEBUG_BUFFER_SIZE 512

typedef struct TSTree TSTree;

typedef unsigned short TSStateId;

typedef enum {
  TSNodeTypeHidden,
  TSNodeTypeAnonymous,
  TSNodeTypeNamed,
} TSNodeType;

typedef struct TSLexer {
  // Public
  void (*start_fn)(struct TSLexer *, TSStateId);
  void (*start_token_fn)(struct TSLexer *);
  bool (*advance_fn)(struct TSLexer *, TSStateId);
  TSTree *(*accept_fn)(struct TSLexer *, TSSymbol, TSNodeType, const char *);

  // Private
  const char *chunk;
  size_t chunk_start;
  size_t chunk_size;

  TSLength current_position;
  TSLength token_end_position;
  TSLength token_start_position;

  size_t lookahead_size;
  int32_t lookahead;

  TSInput input;
  TSDebugger debugger;
  char debug_buffer[TS_DEBUG_BUFFER_SIZE];
} TSLexer;

typedef enum {
  TSParseActionTypeError = 1,
  TSParseActionTypeShift,
  TSParseActionTypeShiftExtra,
  TSParseActionTypeReduce,
  TSParseActionTypeReduceExtra,
  TSParseActionTypeReduceFragile,
  TSParseActionTypeAccept,
} TSParseActionType;

typedef struct {
  TSParseActionType type;
  union {
    TSStateId to_state;
    struct {
      TSSymbol symbol;
      unsigned short child_count;
    };
  } data;
} TSParseAction;

struct TSLanguage {
  size_t symbol_count;
  const char **symbol_names;
  const TSNodeType *node_types;
  const TSParseAction **parse_table;
  const TSStateId *lex_states;
  TSTree *(*lex_fn)(TSLexer *, TSStateId);
};

/*
 *  Lexer Macros
 */

#define START_LEXER()                                        \
  const bool error_mode = (lex_state == ts_lex_state_error); \
  lexer->start_fn(lexer, lex_state);                         \
  int32_t lookahead;                                         \
  next_state:                                                \
  lookahead = lexer->lookahead;

#define START_TOKEN() lexer->start_token_fn(lexer);

#define GO_TO_STATE(state_index) \
  {                              \
    lex_state = state_index;     \
    goto next_state;             \
  }

#define ADVANCE(state_index)               \
  {                                        \
    lexer->advance_fn(lexer, state_index); \
    GO_TO_STATE(state_index);              \
  }

#define ACCEPT_TOKEN(symbol)                                    \
  return lexer->accept_fn(lexer, symbol, ts_node_types[symbol], \
                          ts_symbol_names[symbol]);

#define LEX_ERROR()                      \
  if (error_mode) {                      \
    if (lex_state == ts_lex_state_error) \
      ADVANCE(ts_lex_state_error)        \
    else                                 \
      GO_TO_STATE(ts_lex_state_error)    \
  } else {                               \
    ACCEPT_TOKEN(ts_builtin_sym_error)   \
  }

/*
 *  Parse Table Macros
 */

#define ACTIONS(...)  \
  (TSParseAction[]) { \
    __VA_ARGS__, {    \
      .type = 0       \
    }                 \
  }

#define SHIFT(to_state_value)                                             \
  {                                                                       \
    .type = TSParseActionTypeShift, .data = {.to_state = to_state_value } \
  }

#define SHIFT_EXTRA() \
  { .type = TSParseActionTypeShiftExtra }

#define REDUCE_EXTRA(symbol_val)                                          \
  {                                                                       \
    .type = TSParseActionTypeReduceExtra, .data = {.symbol = symbol_val } \
  }

#define REDUCE(symbol_val, child_count_val)                         \
  {                                                                 \
    .type = TSParseActionTypeReduce,                                \
    .data = {.symbol = symbol_val, .child_count = child_count_val } \
  }

#define REDUCE_FRAGILE(symbol_val, child_count_val)                 \
  {                                                                 \
    .type = TSParseActionTypeReduceFragile,                         \
    .data = {.symbol = symbol_val, .child_count = child_count_val } \
  }

#define ACCEPT_INPUT() \
  { .type = TSParseActionTypeAccept }

#define EXPORT_LANGUAGE(language_name)                       \
  static TSLanguage language = {                             \
    .symbol_count = SYMBOL_COUNT,                            \
    .node_types = ts_node_types,                             \
    .parse_table = (const TSParseAction **)ts_parse_actions, \
    .lex_states = ts_lex_states,                             \
    .symbol_names = ts_symbol_names,                         \
    .lex_fn = ts_lex,                                        \
  };                                                         \
                                                             \
  const TSLanguage *language_name() {                        \
    return &language;                                        \
  }

#ifdef __cplusplus
}
#endif

#endif  // TREE_SITTER_PARSER_H_
