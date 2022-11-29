@namespace();

network ScOSA
{
    parameters:
   		string networkName = default("lab_example");	// name of desired network
        int configNum = default(0);						// initial simulation configuration
        int numConfigs = default(16);					// total number of configurations available
        double simTimeout = default(10);				// length of simulation [s]
//       	ScOSA.networkName = "aton_example"
//		ScOSA.numConfigs = 14
//		ScOSA.simTimeout = 10000
    
    submodules:
        builder: NetworkBuilder;
}

simple NetworkBuilder {}

simple CompNode{
    parameters:
		double timeOut = default(1);					// watchdog timer timeout duration [s]
    gates:
        inout port[1];
        inout taskPort[];
};
simple RouterNode{
    parameters:
        int bufferSize = default(1024);					// size of buffers in the router [bytes]
    	double timeOut = default(1);					// watchdog timer timeout duration [s] 
	gates:
        inout port[31];
};

simple Task{
  	gates:
  	    inout taskPort[1];  
};