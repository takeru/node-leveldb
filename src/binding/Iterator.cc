#include <assert.h>

#include <iostream>

#include <leveldb/iterator.h>
#include <node.h>
#include <v8.h>

#include "DB.h"
#include "helpers.h"
#include "Iterator.h"

#define CHECK_VALID_STATE                                               \
  if (self->it == NULL) {                                               \
    return ThrowError("Illegal state: iterator has been closed");       \
  }

using namespace node_leveldb;

Persistent<FunctionTemplate> Iterator::persistent_function_template;

Iterator::Iterator() {
  this->it = NULL;
}

Iterator::~Iterator() {
  // cleanup the iterator instance
  if (it) {
    delete it;
    it = NULL;
  } // if

  db.Dispose();
}

void Iterator::Init(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> local_function_template = FunctionTemplate::New(New);
  persistent_function_template = Persistent<FunctionTemplate>::New(local_function_template);
  persistent_function_template->InstanceTemplate()->SetInternalFieldCount(1);
  persistent_function_template->SetClassName(String::NewSymbol("Iterator"));

  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "valid", Valid);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "seekToFirst", SeekToFirst);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "seekToLast", SeekToLast);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "seek", Seek);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "next", Next);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "prev", Prev);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "key", key);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "value", value);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "status", status);

  target->Set(String::NewSymbol("Iterator"), persistent_function_template->GetFunction());
}

void Iterator::Close() {
  if (it) {
    delete it;
    it = NULL;
  }
};


//
// Constructor
//

Handle<Value> Iterator::New(const Arguments& args) {
  HandleScope scope;

  assert(args.Length() >= 2);
  assert(args[0]->IsExternal());
  assert(args[1]->IsObject() && DB::HasInstance(args[1]));

  Iterator* iterator = new Iterator();
  iterator->it = (leveldb::Iterator*) Local<External>::Cast(args[0])->Value();
  iterator->db = Persistent<Object>::New(args[1]->ToObject());
  iterator->Wrap(args.This());

  return args.This();
}


//
// Valid
//

Handle<Value> Iterator::Valid(const Arguments& args) {
    HandleScope scope;

    Iterator* self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    return self->it->Valid() ? True() : False();
}


//
// SeekToFirst
//

Handle<Value> Iterator::SeekToFirst(const Arguments& args) {
    HandleScope scope;

    Iterator* self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    Local<Function> callback;
    callback = Local<Function>::Cast(args[0]);

    SeekParams *params = new SeekParams(self, leveldb::Slice(), callback);
    EIO_BeforeSeekToFirst(params);

    return Undefined();
}

void Iterator::EIO_BeforeSeekToFirst(SeekParams *params) {
   eio_custom(EIO_SeekToFirst, EIO_PRI_DEFAULT, EIO_AfterSeek, params);
}

EIO_RETURN_TYPE Iterator::EIO_SeekToFirst(eio_req *req) {
   SeekParams *params = static_cast<SeekParams*>(req->data);
   Iterator *self = params->self;

   self->it->SeekToFirst();

   EIO_RETURN_STMT;
}


//
// SeekToLast
//

Handle<Value> Iterator::SeekToLast(const Arguments& args) {
    Iterator* self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    Local<Function> callback;
    callback = Local<Function>::Cast(args[0]);

    SeekParams *params = new SeekParams(self, leveldb::Slice(), callback);
    EIO_BeforeSeekToLast(params);

    return Undefined();
}

void Iterator::EIO_BeforeSeekToLast(SeekParams *params) {
   eio_custom(EIO_SeekToLast, EIO_PRI_DEFAULT, EIO_AfterSeek, params);
}

EIO_RETURN_TYPE Iterator::EIO_SeekToLast(eio_req *req) {
   SeekParams *params = static_cast<SeekParams*>(req->data);
   Iterator *self = params->self;

   self->it->SeekToLast();

   EIO_RETURN_STMT;
}


//
// Seek
//

Handle<Value> Iterator::Seek(const Arguments& args) {
    Iterator* self = ObjectWrap::Unwrap<Iterator>(args.This());
    HandleScope scope;

    CHECK_VALID_STATE;

    // XXX: Throw away vector that makes JsToSlice work.
    //      the helper needs to be updated.
    std::vector<std::string> strings;

    leveldb::Slice key = JsToSlice(args[0], &strings);
    Local<Function> callback;
    callback = Local<Function>::Cast(args[1]);

    SeekParams *params = new SeekParams(self, key, callback);
    EIO_BeforeSeek(params);

    return scope.Close(Undefined());
}

void Iterator::EIO_BeforeSeek(SeekParams *params) {
   eio_custom(EIO_Seek, EIO_PRI_DEFAULT, EIO_AfterSeek, params);
}

EIO_RETURN_TYPE Iterator::EIO_Seek(eio_req *req) {
   SeekParams *params = static_cast<SeekParams*>(req->data);
   Iterator *self = params->self;

   self->it->Seek(params->key);

   EIO_RETURN_STMT;
}

int Iterator::EIO_AfterSeek(eio_req *req) {
   SeekParams *params = static_cast<SeekParams*>(req->data);
   params->Callback();
   delete params;
   return 0;
}


//
// Next
//

Handle<Value> Iterator::Next(const Arguments& args) {
    Iterator *self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    self->it->Next();

    return Undefined();
}


//
// Prev
//

Handle<Value> Iterator::Prev(const Arguments& args) {
    Iterator *self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    self->it->Prev();

    return Undefined();
}


//
// key
//

Handle<Value> Iterator::key(const Arguments& args) {
    HandleScope scope;
    Iterator *self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    if (!self->it->Valid()) {
      return scope.Close(Null());
    }

    leveldb::Slice k = self->it->key();

    return scope.Close(Bufferize(k.ToString()));
}


//
// value
//

Handle<Value> Iterator::value(const Arguments& args) {
    HandleScope scope;
    Iterator *self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    if (!self->it->Valid())
      return scope.Close(Null());

    leveldb::Slice v = self->it->value();

    return scope.Close(Bufferize(v.ToString()));
}


//
// status
//

Handle<Value> Iterator::status(const Arguments& args) {
    Iterator *self = ObjectWrap::Unwrap<Iterator>(args.This());

    CHECK_VALID_STATE;

    leveldb::Status status = self->it->status();

    return processStatus(status);
}

void Iterator::SeekParams::Callback(Handle<Value> result) {
  if (!callback.IsEmpty()) {
    TryCatch try_catch;
      // no error, callback with no arguments, or result as second argument
      if (result.IsEmpty()) {
        callback->Call(self->handle_, 0, NULL);
      } else {
        Handle<Value> undef = Undefined();
        Handle<Value> argv[] = { undef, result };
        callback->Call(self->handle_, 2, argv);
        }
    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }
  }
}
