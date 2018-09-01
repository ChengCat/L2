#define WORD_SIZE 8

#include "setjmp.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"

//Essentially longjmp and setjmp with a pointer argument
void *jmp_value = NULL;
#define thelongjmp(env, val) (jmp_value = val, longjmp(env, 1))
#define thesetjmp(env) (jmp_value = NULL, setjmp(env), jmp_value)

#include "x86_64_linux_interface.c"
#include "stdlib.h"
#include "stdarg.h"
#include "stdint.h"
typedef int64_t bool;
#define true (~((bool) 0))
#define false ((bool) 0)
#include "list.c"
#include "sexpr.c"
#include "compile_errors.c"
#include "lexer.c"
#include "expressions.c"
#include "preparer.c"
#include "x86_64_generator.c"
#include "x86_64_object.c"
#include "x86_64_assembler.c"

bool equals(void *a, void *b) {
	return a == b;
}

/*
 * Makes a new binary file at the path outbin from the list of primitive
 * expressions, exprs. The resulting binary file executes the list from top to
 * bottom and then makes all the top-level functions visible to the rest of the
 * executable that it is embedded in.
 */

void compile_expressions(unsigned char **objdest, int *objdest_sz, list exprs, region elfreg, jmp_buf *handler) {
	union expression *container = make_begin(), *t;
	{foreach(t, exprs) {
		t->base.parent = container;
	}}
	container->begin.expressions = exprs;
	union expression *root_function = make_function(), *program = root_function;
	put(program, function.expression, container);
	
	visit_expressions(vfind_multiple_definitions, &program, handler);
	visit_expressions(vlink_references, &program, handler);
	visit_expressions(vescape_analysis, &program, NULL);
	program = use_return_value(program, make_reference());
	visit_expressions(vlayout_frames, &program, NULL);
	visit_expressions(vgenerate_references, &program, NULL);
	visit_expressions(vgenerate_continuation_expressions, &program, NULL);
	visit_expressions(vgenerate_literals, &program, NULL);
	visit_expressions(vgenerate_ifs, &program, NULL);
	visit_expressions(vgenerate_function_expressions, &program, NULL);
	list locals = program->function.locals;
	list globals = program->function.parameters;
	program = generate_toplevel(program);
	visit_expressions(vmerge_begins, &program, NULL);
	write_elf(program->begin.expressions, locals, globals, objdest, objdest_sz, elfreg);
}

#include "parser.c"

/*
 * Makes a new binary file at the path outbin from the L2 file at the path inl2.
 * The resulting binary file executes the L2 source file from top to bottom and
 * then makes all the top-level functions visible to the rest of the executable
 * that it is embedded in.
 */

void compile(unsigned char **objdest, int *objdest_sz, char *l2src, int l2src_sz, Symbol *env, region objreg, jmp_buf *handler) {
	list expressions = nil();
	list expansion_lists = nil();
	region syntax_tree_region = create_region(0);
	
	int pos = 0;
	while(after_leading_space(l2src, l2src_sz, &pos)) {
		build_expr_list_handler = handler;
		list sexpr = build_expr_list(l2src, l2src_sz, &pos);
		build_syntax_tree_handler = handler;
		build_syntax_tree_expansion_lists = nil();
		build_syntax_tree(sexpr, append(NULL, &expressions), syntax_tree_region);
		merge_onto(build_syntax_tree_expansion_lists, &expansion_lists);
	}
	
	expand_expressions_handler = handler;
	expand_expressions(&expansion_lists, env, syntax_tree_region);
	compile_expressions(objdest, objdest_sz, expressions, objreg, handler);
	destroy_region(syntax_tree_region);
}

#undef WORD_SIZE
