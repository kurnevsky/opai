Deprecated in favor of opai-rust!

Installation instructions for Linux (debian as an example):

* install build tools:
	aptitude install cmake make

* install libraries needed for the application itself:
	aptitude install libboost-system-dev libboost-thread-dev

* invoke cmake:
	cmake CMakeLists.txt

* invoke make:
	make


That's all, you should now have `opai` executable created and ready to run.

If you want, you can do a simple test for it and run it:
    ./opai
Enter "0 test1234" as a first line then. That line should not be understood by the AI and it should reply with "? 0".
