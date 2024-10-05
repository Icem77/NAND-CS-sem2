# NAND-CS-sem2
Simulation of NAND gate systems 🔌

Project involves implementing a dynamically loaded library in C that handles combinational Boolean circuits made up of NAND gates.

🔌 typedef struct nand nand_t;

  This is the name of the structural type representing a NAND gate.

🔌 nand_t * nand_new(unsigned n);

  The function nand_new creates a new NAND gate with n inputs.

🔌 void nand_delete(nand_t *g);

  The function nand_delete disconnects the input and output signals of the specified gate and then deletes the specified gate,
  freeing all memory it used. It does nothing if called with a NULL pointer. After this function is executed, the pointer passed to it becomes invalid.
  int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k);

🔌 int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k);

  The function nand_connect_nand connects the output of the gate g_out to the kth input of the gate g_in, possibly disconnecting the signal that 
  was previously connected to that input.

🔌 int nand_connect_signal(bool const *s, nand_t *g, unsigned k);

  The function nand_connect_signal connects the Boolean signal s to the kth input of the gate g, possibly disconnecting the signal that was
  previously connected to that input.

🔌 ssize_t nand_evaluate(nand_t **g, bool *s, size_t m);

  The function nand_evaluate determines the output signal values for the given gates and calculates the critical path length.
  The critical path length for a Boolean signal and for a gate with no inputs is zero. The critical path length at the output of a gate
  is 1 + max(S0, S1, S2, ..., Sn-1), where Si is the critical path length at its ith input. The critical path length of the gate circuit
  is the maximum of the critical path lengthsat the outputs of all the given gates.

🔌 ssize_t nand_fan_out(nand_t const *g);

  The function nand_fan_out determines the number of gates connected to the output of a given gate.

🔌 void* nand_input(nand_t const *g, unsigned k);

  The function nand_input returns a pointer to the Boolean signal or the gate connected to the kth input of the gate pointed to by g,
  or NULL if nothing is connected to that input.

🔌 nand_t* nand_output(nand_t const *g, ssize_t k);

  The function nand_output allows iterating over the gates connected to the output of the given gate. The result of this function is undefined
  if its parameters are invalid. If the output of the gate g is connected to multiple inputs of the same gate, that gate appears multiple times in the iteration result.
