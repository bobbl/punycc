/**********************************************************************
 * Steps for writing the code generation for a new architecture
 *
 * Use it as starting point for the backend of a new architecture.
 *
 * SPDX-License-Identifier: 0BSD
 **********************************************************************/




/**********************************************************************
 * Step 1: Preparations for new ISA
 **********************************************************************/

unsigned int buf_size;          /* total size of the buffer */
unsigned char *buf;             /* pointer to the buffer */
unsigned int code_pos;          /* position in the buffer for code generation */
unsigned int num_locals;        /* number of local variables in the current function */
unsigned int num_globals;       /* number of global variables */


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
 * Step 2: Output valid ELF file
 **********************************************************************/

/* Prepare for code generation
   Return the address of the call to main() as a forward reference */
unsigned int emit_begin()
{
    code_pos = 0;
    return 0;
}

/* Finish code generation and return the size of the compiled binary */
unsigned int emit_end()
{
    return code_pos;
}




/**********************************************************************
 * Step 3: Call binary main function
 **********************************************************************/

/* Emit a function that consists of the given binary machine code.
   The parameters are length and pointer to the machine code.
   Return the address of the beginning of the function. */
unsigned int emit_binary_func(unsigned int n, char *s)
{
    return code_pos;
}

/* Call the function at address `ofs`.
   Then remove `pop` values from the stack.
   `save` is the return value from the corresponding emit_pre_call(). It can be
   used to restore the registers that were saved by emit_pre_call().
   Return the address of the call instruction. It must be at least 4 bytes long,
   because it might be overwritten by a pointer if the address of the function
   is not yet known. emit_fix_call() will overwrite this pointer later */
unsigned int emit_call(unsigned int ofs, unsigned int pop, unsigned int save)
{
    return code_pos;
}

/* Write a call instruction at the address from. The target address of the call
   is to. This is used to fix a previous emit_call() where 4 bytes are
   reserved for this call. */
void emit_fix_call(unsigned int from, unsigned int to)
{
}




/**********************************************************************
 * Step 4: Main function with return 40
 **********************************************************************/

/* push value in the accumulator to the value stack */
void emit_push()
{
}

/* set accumulator to a unsigend 32 bit number */
void emit_number(unsigned int x)
{
}

/* Exit from the current function and return the value of the accumulator to
   the caller. Only called on explicit return statements, not at the end of a
   function. */
void emit_return()
{
}




/**********************************************************************
 * Step 5: Call library function to exit
 **********************************************************************/

/* Emit code to put the value from the accumulator at the right location for
   an function argument. That could be a stack or a register. It is called in
   order of the arguments. */
void emit_arg()
{
}




/**********************************************************************
 * Step 6: String constants
 **********************************************************************/

/* Store string somewhere in the binary and set accumulator to its address.
   To simplify the alignment code here, it is guaranteed that 4 zero bytes
   are following after the end of s. */
void emit_string(unsigned int len, char *s)
{
}




/**********************************************************************
 * Step 7: Global variables
 **********************************************************************/

/* Reserve memory for a global variable and return its address */
unsigned int emit_global_var()
{
    num_globals = num_globals + 1;
    return num_globals - 1;
}

/* Store accumulator in a global(1) or local(0) variable with address `ofs`
   Function arguments have global=0 and ofs=1,2,3,... For other variables,
   ofs is the return value of emit_local_var() or emit_global_var() */
void emit_store(unsigned int global, unsigned int ofs)
{
}

/* Load accumulator from a global(1) or local(0) variable with address `ofs`
   Function arguments have global=0 and ofs=1,2,3,... For other variables,
   ofs is the return value of emit_local_var() or emit_global_var() */
void emit_load(unsigned int global, unsigned int ofs)
{
}




/**********************************************************************
 * Step 8: Arithmetic operations
 **********************************************************************/

/* Pop one value from the stack.
   Combine it with the accumulator and store the result in the accumulator.

   operation
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
void emit_operation(unsigned int operation)
{
}




/**********************************************************************
 * Step 9: Compare operations
 **********************************************************************/

/* Pop one value from the stack and compare it with the accumulator.
   Store 1 in the accumulator if the comparison is true, otherwise 0.

   condition (same as in emit_if()
     0  tos == accu
     1  tos != accu
     2  tos <  accu
     3  tos >= accu
     4  tos >  accu
     5  tos <= accu
*/
void emit_comp(unsigned int condition)
{
}




/**********************************************************************
 * Step 10: if and while
 **********************************************************************/

/* Called at the beginning of a while loop, before the condition.
   The return value is forwarded to emit_jump_and_fix_jump_here() at the end of
   the loop. It must not be 0, because that means emit_jump_and_fix_jump_here()
   is called between a then and else branch of a if statement. Typically the
   return value is code_pos, the address of the top of the loop.
*/
unsigned int emit_pre_while()
{
    return code_pos;
}

/* Called after a if or while condition.
   Emit code for a branch if condition is NOT true.
   Return address where the branch target address will be written later.

   condition (same as in emit_comp()
     0  tos == accu
     1  tos != accu
     2  tos <  accu
     3  tos >= accu
     4  tos >  accu
     5  tos <= accu
*/
unsigned int emit_if(unsigned int condition)
{
    return code_pos;
}

/* Called at the end of a then branch without an else branch.
   Used to fix the branch instruction at the end of the then branch. It should
   point to the current address.
   The address of the branch instruction is given by the parameter.
*/
void emit_then_end(unsigned int insn_pos)
{
}

/* Called at the end of an else branch.
   Used to fix the jump instruction at the end of the then branch. It should
   point to the current address.
   The address of the jump instruction is given by the parameter.
*/
void emit_else_end(unsigned int insn_pos)
{
}

/* Called between then and else branch of an if statement.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address.
   Return address where the jump target address will be written later  */
unsigned int emit_then_else(unsigned int insn_pos)
{
    return code_pos;
}

/* Called at the end of a while loop with the value returned by
   emit_pre_while(). Typically it is the address of the top of the loop.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address. */
void emit_loop(unsigned int destination, unsigned int insn_pos)
{
}




/**********************************************************************
 * Step 11: Access function arguments
 **********************************************************************/

/* Emit code at the beginning of a function.
   The parameter is the number of arguments for the function.
   The return value is the address of the function entry point. */
unsigned int emit_func_begin(unsigned int n)
{
    return code_pos;
}

/* Emit code at the end of a function.
   Typically simply emit_return() is called here. */
void emit_func_end()
{
    emit_return();
}




/**********************************************************************
 * Step 12: Local variables
 **********************************************************************/

/* Reserve memory for a local variable and return its address.
   Function arguments have the addresses 1, 2, 3, ... Therefore use an address
   scheme that fits this.
   If `init` is not 0, an initial value is in the accumulator and the variable
   must be set accordingly. */
unsigned int emit_local_var(unsigned int init)
{
    num_locals = num_locals + 1;
    return num_locals;
}

/* Called at the beginning of a scope (`{`).
   The value that is returned will be the parameter for the corresponding
   emit_scope_end().
   Typically the stack position is returned to remove temporary variables
   from the stack at the end of the scope. */
unsigned int emit_scope_begin()
{
    return 0;
}

/* Called at end of a scope (`}`).
   The parameter is the return value of the corresponding emit_scope_begin(). */
void emit_scope_end(unsigned int stack_pos)
{
}




/**********************************************************************
 * Step 13: Function result as argument
 **********************************************************************/

/* Called before pushing the arguments of a function call.
   The return value is forwarded to emit_call().
   Used in emit_rv32.c to save registers before using them for the function 
   arguments. The return value tells emit_call() how many registers have to
   be restored. */
unsigned int emit_pre_call()
{
    return 0;
}




/**********************************************************************
 * Step 14: Arrays
 **********************************************************************/

/* input:  global(1) or local(0) variable used as a pointer
           accumulator contains the index into the array the pointer points to
   output: push pointer to the array element */
void emit_index_push(unsigned int global, unsigned int ofs)
{
}

/* pop address (from emit_index_push()) and store lowest byte of accumulator
   there */
void emit_pop_store_array()
{
}

/* Read global(1) or local(0) variable with address `ofs` as base pointer.
   Add index (in accumulator) to the base pointer.
   Load byte from the computed address and store in accumulator.  */
void emit_index_load_array(unsigned int global, unsigned int ofs)
{
}



