#include "bool.h"
#include "malloc.h"
#include "lists.h"
#include "stringBuffers.h"
#include "sockets.h"
//#include "threading.h"



int main()
//@ requires true;
//@ ensures true;
{
	struct socket *s = create_client_socket(12345);
	struct reader *r = socket_get_reader(s);
	struct writer *w = socket_get_writer(s);
	bool stop = false;
	bool test = true;

	struct string_buffer *line = create_string_buffer();

	struct string_buffer *nick = create_string_buffer();
	struct string_buffer *text = create_string_buffer();

	bool result = false;

	reader_read_line(r,line);
	reader_read_line(r,line);
	writer_write_string(w,"BoT\r\n");
	while(! stop)
		//@ invariant reader(r) &*& writer(w) &*& stop ? emp : (string_buffer(line) &*& string_buffer(nick) &*& string_buffer(text));
	{
		string_buffer_dispose(line); //remove to introduce memory leak!
		line = create_string_buffer();
		reader_read_line(r,line);
		result = string_buffer_split_string(line, " says: ", nick, text);
		test = string_buffer_equals_string(nick,"BoT");
		if(result && !test )
		{
			test = string_buffer_equals_string(text,"!hello");
			if(test)
			{
				writer_write_string(w,"Hello ");
				writer_write_string_buffer(w,nick);
				writer_write_string(w,"!\r\n");
			}
			else {
				test = string_buffer_equals_string(text,"!quit");
				if(test)
				{
					writer_write_string(w,"Byebye!\r\n");
					stop = true;
					string_buffer_dispose(line);
					string_buffer_dispose(nick);
					string_buffer_dispose(text);
				}
			}
		}
	}


	socket_close(s);

	return 1;
}