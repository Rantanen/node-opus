
#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include "../deps/opus/include/opus.h"
#include "common.h"
#include <nan.h>

#include <string.h>

using namespace node;
using namespace v8;

class OpusEncoder : public ObjectWrap {
	private:
		OpusEncoder* encoder;
		OpusDecoder* decoder;

		opus_int32 rate;
		int channels;
		int application;

		unsigned char compressedBuffer[ 1024 ];
		opus_int16 frameBuffer[ 5760 * 2 ];

	protected:
		int EnsureEncoder() {
			if( encoder != NULL ) return 0;
			int error;
			encoder = opus_encoder_create( rate, channels, application, &error );
			return error;
		}
		int EnsureDecoder() {
			if( decoder != NULL ) return 0;
			int error;
			decoder = opus_decoder_create( rate, channels, &error );
			return error;
		}

	public:
	   	OpusEncoder( opus_int32 rate, int channels, int application ):
			encoder( NULL ), decoder( NULL ),
			rate( rate ), channels( channels ), application( application ) {
		}

		~OpusEncoder() {
			if( encoder != NULL )
				opus_encoder_destroy( encoder );
			if( decoder != NULL )
				opus_decoder_destroy( decoder );

			encoder = NULL;
			decoder = NULL;
		}

		static NAN_METHOD(Encode) {
			NanScope();

			REQ_OBJ_ARG( 0, pcmBuffer );
			REQ_INT_ARG( 1, frameSize );
			OPT_INT_ARG( 2, compressedSize, 512 );

			// Read the PCM data.
			char* pcmData = Buffer::Data(pcmBuffer);
			opus_int16* pcm = reinterpret_cast<opus_int16*>( pcmData );

			// Unwrap the encoder.
			OpusEncoder* self = ObjectWrap::Unwrap<OpusEncoder>( args.This() );
			self->EnsureEncoder();

			// Encode the samples.
			int compressedLength = opus_encode( self->encoder, pcm, frameSize, &(self->compressedBuffer[0]), compressedSize );

			// Create a new result buffer.
			Local<Object> actualBuffer = NanNewBufferHandle(reinterpret_cast<char*>(self->compressedBuffer), compressedLength );

			NanReturnValue( actualBuffer );
		}

		static NAN_METHOD(Decode) {
			NanScope();

			REQ_OBJ_ARG( 0, compressedBuffer );
			REQ_INT_ARG( 1, frameSize );

			// Read the compressed data.
			unsigned char* compressedData = (unsigned char*)Buffer::Data( compressedBuffer );
			size_t compressedDataLength = Buffer::Length( compressedBuffer );

			OpusEncoder* self = ObjectWrap::Unwrap<OpusEncoder>( args.This() );
			self->EnsureDecoder();

			// Encode the samples.
			int decoded = opus_decode(
					self->decoder,
					compressedData,
					compressedDataLength,
					&(self->frameBuffer[0]),
				   	frameSize, /* decode_fex */ 0 );

			// Create a new result buffer.
			Local<Object> actualBuffer = NanNewBufferHandle(reinterpret_cast<char*>(self->frameBuffer), decoded);


			NanReturnValue( actualBuffer );
		}

		static NAN_METHOD(New) {
			NanScope();

			if( !args.IsConstructCall()) {
				NanThrowTypeError("Use the new operator to construct the OpusEncoder.");
				NanReturnUndefined();
			}

			OPT_INT_ARG(0, rate, 42000);
			OPT_INT_ARG(1, channels, 1);
			OPT_INT_ARG(2, application, OPUS_APPLICATION_AUDIO);

			OpusEncoder* encoder = new OpusEncoder( rate, channels, application );

			encoder->Wrap( args.This() );
			NanReturnValue(args.This());
		}

		static void Init(Handle<Object> exports) {
			Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
			tpl->SetClassName(NanNew<String>("OpusEncoder"));
			tpl->InstanceTemplate()->SetInternalFieldCount(1);

			tpl->PrototypeTemplate()->Set( NanNew<String>("encode"),
				NanNew<FunctionTemplate>( Encode )->GetFunction() );

			tpl->PrototypeTemplate()->Set( NanNew<String>("decode"),
				NanNew<FunctionTemplate>( Decode )->GetFunction() );

			//v8::Persistent<v8::FunctionTemplate> constructor;
			//NanAssignPersistent(constructor, tpl);
			exports->Set(NanNew<String>("OpusEncoder"), tpl->GetFunction());
		}
};


void NodeInit(Handle<Object> exports) {
	OpusEncoder::Init( exports );
}

NODE_MODULE(node_opus, NodeInit)
