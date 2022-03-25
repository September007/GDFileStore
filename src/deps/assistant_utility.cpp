#include<assistant_utility.h>

buffer::buffer(const string& s) {
	this->_expand_prepare(s.size());
	WriteSequence(*this, s.c_str(), s.size());
}

bool buffer::_expand_prepare(int addtionalLength) {
	if (addtionalLength + length > buffer_length)
		try {
		auto newBufferLength = (addtionalLength + length) * incre_factor;
		auto new_data = (UnitType*)malloc(
			sizeof(UnitType) * newBufferLength);  // UnitType[newBufferLength];
		memcpy(new_data, data, length);
		buffer_length = newBufferLength;
		free(data);
		data = new_data;
	}
	catch (std::exception& msg) {
		GetLogger("buffer", false)->error("{} at {}:{},", __FUNCTION__, __FILE__, __LINE__, msg.what());
		return false;
	}
	return true;
}

bool buffer::Read(buffer& buf, buffer* rbuf) {
	int buf_length;
	::Read(buf, &buf_length);
	rbuf->_expand_prepare(buf_length);
	::ReadSequence(buf, rbuf->data, buf_length);
	return true;
}

void buffer::Write(buffer& buf, buffer* wbuf) {
	::Write(buf, &wbuf->buffer_length);
	::WriteSequence(buf, wbuf->data, wbuf->buffer_length);
	return ;
}

bool Slice::Read(buffer& buf, Slice* sli) {
	sli->data= make_shared<buffer>();
	::Read(buf, &sli->start);
	::Read(buf, &sli->end);
	auto len = sli->end - sli->start;
	sli->data->_expand_prepare(len);
	::ReadSequence(buf, sli->data->data, len);
	sli->data->length += len;
	return true;
}

void Slice::Write(buffer& buf, Slice* s) {
	::Write(buf, &s->start);
	::Write(buf, &s->end);
	::WriteSequence(buf, s->data->data + s->start, s->GetSize());
	return;
}
