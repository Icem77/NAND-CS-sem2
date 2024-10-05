#include "nand.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// connection between gate and signal source
typedef struct cable 
{
    nand_t *nand_signal_source;
    const bool *bool_signal_source;
} cable_t;

typedef struct nand 
{
    unsigned int number_of_inputs;
    cable_t **inputs;
    ssize_t number_of_outputs;
    nand_t **outputs;
    bool signal;
    ssize_t critical_path;
} nand_t;

/**
    a, b - int values

    Returns the greatest of the argument values.
*/
int max(int a, int b)
{
    if (a > b) 
    { 
        return a; 
    }
    else 
    {
        return b;
    }
}

/**
    input_owner - nand gate which recieves a signal
    cable - connection between 'input_owner' and signal source

    Properly unplugs cable 'cable' from the input of 'input_owner'.
*/
void unplug_input(nand_t *input_owner, cable_t *cable) 
{   
    // check if cable connects 'input_owner' with another gate
    if (cable->nand_signal_source != NULL)
    {   
        int i = 0;
        
        // search for connection with 'input_owner'
        while (cable->nand_signal_source->outputs[i] != input_owner)
        {
            i++;
        }

        // shift array elements behind found connection to the left
        while (i < cable->nand_signal_source->number_of_outputs - 1)
        {
            cable->nand_signal_source->outputs[i] = 
                                cable->nand_signal_source->outputs[i + 1];
            i++;
        }
        cable->nand_signal_source->outputs[i] = NULL;
    }

    free(cable);
    cable = NULL;
}

/**
    output_owner - pointer to the gate which is source of the signal
    output_reciever - pointer to the gate which recieves the signal

    Properly unplugs output of gate 'signal_source' from gate 'signal_reciever'.

    NOTE: This operation is done only when 'output_owner' gate is beeing deleted
    so there is no need to shift elements in it's 'outputs' array
*/
void unplug_output(nand_t *signal_reciever, nand_t *signal_source)
{   
    for (unsigned i = 0; i < signal_reciever->number_of_inputs; i++)
    {   
        // search for connection with signal source
        if (signal_reciever->inputs[i] != NULL &&
            signal_reciever->inputs[i]->nand_signal_source == signal_source)
        {
            free(signal_reciever->inputs[i]);
            signal_reciever->inputs[i] = NULL;
        }
    }
}

/**
    n - number of input sockets 

    Creates new nand gate with 'n' inputs and 2 initial, empty output sockets 
    and returns pointer to the gate. If an allocation memory error occurs, it 
    returns NULL pointer and sets 'errno' to 'EMOMEM'.
*/
nand_t* nand_new(unsigned n) 
{   
    // creating gate 
   	nand_t *new_gate = (nand_t *) malloc(sizeof(nand_t));
    if (!new_gate) 
    {
    	errno = ENOMEM;
    	return NULL;
    }

    // creating array of inputs with given size 'n'
    new_gate->inputs = (cable_t **) calloc(n, sizeof(cable_t *));
    if (!(new_gate->inputs)) 
    {
        free(new_gate);
        errno = ENOMEM;
        return NULL;
    }

    // creating array of outputs with default size of 2
    new_gate->outputs = (nand_t **) calloc(2, sizeof(nand_t *));
    if (!(new_gate->outputs)) 
    { 
        free(new_gate->inputs);
        free(new_gate);
        errno = ENOMEM;
        return NULL; 
    }
    
    // setting gate parameters
    new_gate->number_of_inputs = n;
    new_gate->number_of_outputs = 2;
    new_gate->signal = false;
    new_gate->critical_path = 0;

    return new_gate;
}
/**
    g - pointer to nand gate

    Unplugs all inputs and outputs of gate 'g' and sets free all its memory.
    Sets 'g' pointer to NULL.
*/
void nand_delete(nand_t *g) {

    if (g != NULL)
    {   
        // unpluging inputs
        for (unsigned i = 0; i < g->number_of_inputs; i++) 
        {   
            if (g->inputs[i] != NULL)
            {
                unplug_input(g, g->inputs[i]);
            }
        }

        // unpluging outputs
        for (int j = 0; j < g->number_of_outputs && g->outputs[j] != NULL; j++)
        {   
            if (g->outputs[j] != NULL)
            {
                unplug_output(g->outputs[j], g);
            }
        }

        free(g->inputs);
        free(g->outputs);
        free(g);
        g = NULL;
    }
}

/**
    s - pointer to bool value
    g - pointer to nand gate
    k - index of input of gate 'g'

    Connects bool signal 's' with 'k' input of gate 'g'.
    If something was connected to this input before it deletes this connection.
    If 'g_out' or 'g_in' points to NULL or k is too big it sets 'errno' to
    EINVAL and returns '-1'. If memory allocation error occurs it sets 'errno'
    to ENOMEM and returns '-1'.
*/
int nand_connect_signal(bool const *s, nand_t *g, unsigned k)
{   
    // incorrect parameters handling
    if (s == NULL || g == NULL || g->number_of_inputs <= k)
    {
        errno = EINVAL;
        return -1;
    }

    // create new connection
    cable_t *new_cable = (cable_t *) malloc(sizeof(cable_t));
    if (new_cable == NULL) 
    {
    	errno = ENOMEM;
    	return -1;
    }

    new_cable->nand_signal_source = NULL;
    new_cable->bool_signal_source = s;

    // eventually unplug previous connection
    if (g->inputs[k] != NULL) 
    {
        unplug_input(g, g->inputs[k]);
    }

    // set new connection
    g->inputs[k] = new_cable;

    return 0;
}

/**
    g_out - pointer to nand gate which is source of a signal
    g_in - pointer to nand gate which will recieve the signal
    k - index of input of gate 'g_in'

    Connects output of gate 'g_out' with 'k' input of gate 'g_in'.
    If something was connected to this input before it deletes this connection.
    If 'g_out' or 'g_in' points to NULL or k is too big it sets 'errno' to
    EINVAL and returns '-1'. If memory allocation error occurs it sets 'errno'
    to ENOMEM and returns '-1'.
*/
int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k)
{   
    // incorrect parameters handling
    if (g_out == NULL || g_in == NULL || g_in->number_of_inputs <= k)
    {
        errno = EINVAL;
        return -1;
    }

    // eventually unplug previous connection
    if (g_in->inputs[k] != NULL)
    {
        unplug_input(g_in, g_in->inputs[k]);
    }

    // search for the first empty output socket of 'g_out'
    int x = g_out->number_of_outputs - 1;
    while (x >= 0 && g_out->outputs[x] == NULL) 
    {
    	x--;
    }

    // check if output array is big enough for a new connection
    if (x == g_out->number_of_outputs - 1)
    {
        nand_t **ptr = (nand_t **) realloc(g_out->outputs,
                        (g_out->number_of_outputs * 2) * sizeof(nand_t *));

        if (ptr == NULL) 
        {   
            errno = ENOMEM;
            return -1;
        }

        g_out->outputs = ptr;
        g_out->number_of_outputs = g_out->number_of_outputs * 2;

        // initialize newly added memory with NULLs
        for (int i = g_out->number_of_outputs / 2; 
            i < g_out->number_of_outputs; i++)
        {
            g_out->outputs[i] = NULL;
        }
    }

    // create new connection
    cable_t *new_cable = (cable_t *) malloc(sizeof(cable_t));
    if(new_cable == NULL) 
    {   
        errno = ENOMEM;
        return -1;
    }

    new_cable->bool_signal_source = NULL;
    new_cable->nand_signal_source = g_out;

    // add new connection to the output arrary of source gate
    g_out->outputs[x + 1] = g_in;

    // add new connection to the input array of signal reciever
    g_in->inputs[k] = new_cable;

    return 0;
}

/**
    g - pointer to nand gate
    main_path - pointer to int value

    Returns output signal of gate 'g' and sets 'main_path' to critical path
    of gate 'g'. Sets 'main_path' to '-1' if it gets stuck in a loop.
*/
bool calculate_critical_path_and_signal(nand_t *g, int *main_path)
{   
    // we have not visited this gate yet
    if (g->critical_path == 0)
    {
        // gate has no inputs - critical path is 0, signal is false
        if (g->number_of_inputs == 0) 
        {
        	g->critical_path = 0;
        	*main_path = 0;
        	return false;
        }
        else
        {
            g->signal = false;

            // find output signal and critical path for all input gates
            for (unsigned i = 0; i < g->number_of_inputs; i++)
            {   
                // nothing is connected to the input socket
                // unable to calculate signal and critical path
                if (g->inputs[i] == NULL) 
                { 
                	*main_path = -1;
                	return false; 
                }

                // bool signal is connected to the input socket
                if (g->inputs[i]->nand_signal_source == NULL)
                {   
                    // if signal is 'false' then gate output can only be true
                    if (*(g->inputs[i]->bool_signal_source) == false) 
                    { 
                        g->signal = true;
                    }
                }
                else // nand gate is connected to the input socket
                {   
                    // set critical path of gate to '-1 to detect loop
                    int critical_path_holder = g->critical_path;
                    g->critical_path = -1;

                    if (calculate_critical_path_and_signal
                        (g->inputs[i]->nand_signal_source, main_path) == false) 
                    { 
                    	g->signal = true;
                    }

                    // check result of calculation
                    if (*main_path == -1) 
                    {
                    	return false;
                    }

                    g->critical_path = critical_path_holder;

                    // update critical path for gate 'g'
                    g->critical_path = max(g->critical_path, *main_path);
                }
            }

            g->critical_path += 1;
            *main_path = g->critical_path;
            return g->signal;
        }
    }
    else if (g->critical_path < 0) // we are in a loop
    {
        *main_path = -1;
        return false;
    }
    else // signal and path for this gate were already calculated
    {
        *main_path = g->critical_path;
        return g->signal;
    }
}

/**
    g - pointer to nand gate

    Sets critical_path of gate 'g' and gates connected to its input to 0.
*/
void clean_after_evaluation(nand_t *g)
{   
    if (g->critical_path != 0)
    {
        g->critical_path = 0;

        for (unsigned i = 0; i < g->number_of_inputs; i++)
        {
            if (g->inputs[i] != NULL)
            {   
                if (g->inputs[i]->bool_signal_source == NULL)
                {
                    clean_after_evaluation(g->inputs[i]->nand_signal_source);
                }
            
            }
        }
    }
}

/**
    g - array of pointers to nand gates
    s - bool array
    m - size of 'g' and 's'

    Calculates output signal of gates in array 'g' and returns critical path of
    given gates system. Calculated signal of gate g[x] is stored in s[x].
    If 'g' or 's' points to NULL or 'm' is equal to '0' then it sets 'errno'
    to EINVAL and returns '-1'. If calculation ends in failure it returns '-1'.
    Calculating the output signals and critical path may end in failure if:
    - there is an input without gate or signal connected to it
    - gates do not create combinational circuit (we got stuck in a loop)
    - memory allocation error occurs
*/
ssize_t nand_evaluate(nand_t **g, bool *s, size_t m)
{   
    // handling wrong parameters
    if (g == NULL || s == NULL || m == 0) 
    { 
        errno = EINVAL;
        return -1;
    }

    int critical_path = 0;
    int input_path = 0;

    // for every gate in 'g'
    for (size_t i = 0; i < m; i++)
    {   
        if (g[i] != NULL)
        {
            s[i] = calculate_critical_path_and_signal(g[i], &input_path);

            // check if calculating critical path and signal ended in failure
            if (input_path == -1)
            {   
                for (size_t j = 0; j < m; j++)
                {   
                    if (g[j])
                    {
                        clean_after_evaluation(g[j]);
                    }
                    
                }
                errno = ECANCELED;
                return -1;
            }
            else
            {   
                // update function's result
                critical_path = max(input_path, critical_path);
            }
        }
        else
        {
            for (size_t j = 0; j < m; j++)
            {   
                if (g[j])
                {
                    clean_after_evaluation(g[j]);
                }
            }
            errno = EINVAL;
            return -1;
        }
    }

    for (size_t j = 0; j < m; j++)
    {
        if (g[j])
        {
            clean_after_evaluation(g[j]);
        }
    }

    return critical_path;
}

/**
    g - pointer to nand gate

    Returns number of inputs connected to output of gate 'g'.
    If 'g' points to NULL it sets 'errno' to EINVAL and returns '-1'.
*/
ssize_t nand_fan_out(nand_t const *g)
{
    if (g == NULL) 
    { 
        errno = EINVAL;
        return -1; 
    }

    // search for first non empty output socket
    int i = g->number_of_outputs - 1;
    while(i >= 0 && g->outputs[i] == NULL)
    {
        i--;
    }

    return i + 1;
}

/**
    g - pointer to nand gate
    k - index of input of gate 'g'

    Return pointer to bool signal or nand gate connected to the 'k'
    input of gate 'g'. If 'k' is too big or g points to NULL it sets
    'errno' to EINVAL and returns NULL. If nothing is connected to the
    'k' input of gate 'g' it sets 'errno' to '0' and returns NULL.
*/
void* nand_input(nand_t const *g, unsigned k)
{   
    // handle incorrect parameters
    if (g == NULL || g->number_of_inputs <= k)
    {
        errno = EINVAL;
        return NULL;
    }
    if (g->inputs[k] == NULL) 
    { 
        errno = 0;
        return NULL;
    }

    if (g->inputs[k]->bool_signal_source == NULL)
    {
        return (nand_t *) g->inputs[k]->nand_signal_source;
    }
    else
    {
         return (bool *) g->inputs[k]->bool_signal_source;
    }
}

/**
    g - pointer to nand gate
    k - value in range from 0 to one less than the number of gates connected
    to the output of gate 'g'

    Enables iteration over gates connected to the output of gate 'g'.
    If some gate is connected to the output of gate 'g' in multiple input
    sockets it occurs as many times during the iteration.
*/
nand_t* nand_output(nand_t const *g, ssize_t k)
{   
    // handle incorrect parameters
    if (g == NULL || g->number_of_outputs <= k)
    { 
        return NULL;
    }

    return g->outputs[k];
}