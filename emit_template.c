/**********************************************************************
 * Code Generation Template
 *
 * Use it as starting point for the backend of a new architecture.
 *
 * SPDX-License-Identifier: 0BSD
 **********************************************************************/



/**********************************************************************
 * Required global variables
 **********************************************************************/

/* constants */
unsigned int buf_size;      /* total size of the buffer */

/* global variables */
unsigned char *buf;     /* pointer to the buffer */
unsigned int code_pos;      /* position in the buffer for code generation */




/**********************************************************************
 * Recommended names for global variables
 **********************************************************************/

unsigned int stack_pos;     /* current stack pointer */
unsigned int num_params;    /* number of function parameters */
unsigned int num_locals;    /* number of local variables in the current function */
unsigned int num_globals;   /* number of global variables */




/**********************************************************************
 * Helper
 **********************************************************************
 * These functions are also used by punycc.c and identical in every
 * backend. A forward reference to this code in punycc.c would be an
 * option. But forward references lead to larger binaries an therefore
 * the code is copied to every backend.
 **********************************************************************/

/* write a 32 bit number to a char array */
void set_32bit(unsigned char *p, unsigned int x)
{
    p[0] = x;
    p[1] = x >> 8;
    p[2] = x >> 16;
    p[3] = x >> 24;
}

/* read 32 bit number from a char array */
unsigned int get_32bit(unsigned char *p)
{
    return p[0] +
          (p[1] << 8) +
          (p[2] << 16) +
          (p[3] << 24);
}




/**********************************************************************
 * Push basic values
 **********************************************************************/


/* push value in the accumulator to the value stack */
void emit_push();


/* set accumulator to a unsigend 32 bit number */
void emit_number(unsigned int x);


/* Store string somewhere in the binary and set accumulator to its address.
   To simplify alignment, 4 zero bytes are following after the end of s. */
void emit_string(unsigned int len, char *s);




/**********************************************************************
 * Read/write variables and arrays
 **********************************************************************/


/* store accumulator in a global(1) or local(0) variable with address `ofs` */
void emit_store(unsigned int global, unsigned int ofs);


/* load accumulator from a global(1) or local(0) variable with address `ofs` */
void emit_load(unsigned int global, unsigned int ofs);


/* input:  global(1) or local(0) variable used as a pointer
           accumulator contains the index into the array the pointer points to
   output: push pointer to the array element */
void emit_index_push(unsigned int global, unsigned int ofs);


/* pop address (from emit_index_push()) and store lowest byte of accumulator
   there */
void emit_pop_store_array();

/* Read global(1) or local(0) variable with address `ofs` as base pointer.
   Add index (in accumulator) to the base pointer.
   Load byte from the computed address and store in accumulator.  */
void emit_index_load_array(unsigned int global, unsigned int ofs);




/**********************************************************************
 * Operations combining accumulator and top of stack
 **********************************************************************/


/* Pop one value from the stack.
   Combine it with the accumulator and store the result in the accumulator.

   operations
      1  <<  shift left
      2  >>  shift right
      3  -   subtract
      4  |   or
      5  ^   xor
      6  +   add
      7  &   and
      8  *   multiply
      9  /   divide
     10  %   modulo
*/
void emit_operation(unsigned int op);


/* Pop one value from the stack and compare it with the accumulator.
   Store 1 in the accumulator if the comparison is true, otherwise 0.

   condition
     16  tos == accu
     17  tos != accu
     18  tos <  accu
     19  tos >= accu
     20  tos >  accu
     21  tos <= accu
*/
void emit_comp(unsigned int op);




/**********************************************************************
 * If and while
 **********************************************************************/


/* Called at the beginning of a while loop, before the condition.
   The return value is forwarded to emit_jump_and_fix_jump_here() at the end of
   the loop. It must not be 0, because that means emit_jump_and_fix_jump_here()
   is called between a then and else branch of a if statement. Typically the
   return value is code_pos, the address of the top of the loop.
*/
unsigned int emit_pre_while()


/* Emit code for a branch if condition is NOT true.
   Return address where the branch target address will be written later.

   condition
      0  accu == 0
     16  tos == accu
     17  tos != accu
     18  tos <  accu
     19  tos >= accu
     20  tos >  accu
     21  tos <= accu
*/
unsigned int emit_if(unsigned int condition);


/* Called at the end of a then branch without an else branch.
   Used to fix the branch instruction at the end of the then branch. It should
   point to the current address.
   The address of the branch instruction is given by the parameter.
*/
void emit_fix_branch_here(unsigned int insn_pos);


/* Called at the end of an else branch.
   Used to fix the jump instruction at the end of the then branch. It should
   point to the current address.
   The address of the jump instruction is given by the parameter.
*/
void emit_fix_jump_here(unsigned int insn_pos);


/* Called between then and else branch of an if statement when destination==0.
   Called at the end of a while loop with the value returned by
   emit_pre_while(). This value must not be 0. Typically it is the address of
   the top of the loop.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address. */
unsigned int emit_jump_and_fix_branch_here(unsigned int destination, unsigned int insn_pos);




/**********************************************************************
 * Function call with arguments
 **********************************************************************/


/* Called before pushing the arguments of a function call.
   The return value is forwarded to emit_call().
   Used in emit_rv32.c to save registers before using them for the function 
   arguments. The return value tells emit_call() how many registers have to
   be restored. */
unsigned int emit_pre_call();


/* Emit code to put the value from the accumulator at the right location for
   an function argument. That could be a stack or a register. It is called in
   order of the arguments. For three arguments that means emit_arg(0) then
   emit_arg(1) then emit_arg(2). */
void emit_arg(unsigned int i);


/* Call the function at address `ofs`.
   Then remove `pop` values from the stack.
   `save` is the return value from the corresponding emit_pre_call(). It can be
   used to restore the registers that were saved by emit_pre_call().
   Return the address of the call instruction. It must be at least 4 bytes long,
   because it might be overwritten by a pointer if the address of the function
   is not yet known. emit_fix_call() will overwrite this pointer later */
unsigned int emit_call(unsigned int ofs, unsigned int pop, unsigned int save);


/* Write a call instruction at the address from. The target address of the call
   is to. This is used to fix a prevoiusly emit_call() where 4 bytes are
   reserved for this call. */
unsigned int emit_fix_call(unsigned int from, unsigned int to);




/**********************************************************************
 * Declare variables
 **********************************************************************/


/* Reserve memory for a local variable and return its address.
  If `init` is not 0, an initial value is in the accumulator and the variable
  must be set accordingly. */
unsigned int emit_local_var(unsigned int init);

/* Reserve memory for a global variable and return its address */
unsigned int emit_global_var();




/**********************************************************************
 * Begin and end of sections
 **********************************************************************/


/* Exit from the current function and return the value of the accumulator to
   the caller. Only called on explicit return statements, not at the end of a
   function. */
void emit_return();


/* Emit a function that consisits of the given binary machine code.
   The parameters are length and pointer to the machine code. */
void emit_binary_func(unsigned int n, char *s);


/* Emit code at the beginning of a function.
   The parameter is the number of arguments for the function.
   The return value is the address of the function entry point. */
unsigned int emit_func_begin(unsigned int n);


/* Emit code at the end of a function.
   Typically simply emit_return() is called here. */
void emit_func_end();
{
    emit_return();
}



/* Called at the beginning of a scope (`{`).
   The value that is returned will be the parameter for the corresponding
   emit_scope_end().
   Typically the stack position is returned to remove temporary variables
   from the stack at the end of the scope. */
unsigned int emit_scope_begin();


/* Called at end of a scope (`}`).
   The parameter is the return value of the corresponding emit_scope_begin(). */
void emit_scope_end(unsigned int stack_pos);


/* Prepare for code generation
   Return the address of the call to main() as a forward reference */
unsigned int emit_begin();


/* Finish code generation and return the size of the compiled binary */
unsigned int emit_end();

