binding = require '../leveldb.node'

###

    An iterator allows sequential and random access to the database.

    Usage:

        var leveldb = require('leveldb');

        var db = leveldb.open('/tmp/test.db')
          , it = db.iterator();

        // iterator is initially invalid
        it.firstSync();

        // get a key
        it.get('foobar');

###

exports.Iterator = class Iterator

  noop = (err) -> throw err if err


  ###

      Constructor.

      @param {Native} self The underlying iterator object.

  ###

  constructor: (@self) ->

  ###

      Apply a callback over a range.

      The iterator will be positioned at the given key or the first key if
      not given, then the callback will be applied on each record moving
      forward until the iterator is positioned at the limit key or at an
      invalid key. Stops on first error.

      @param {String|Buffer} [startKey] Optional start key (inclusive) from
        which to begin applying the callback. If not given, defaults to the
        first key.
      @param {String|Buffer} [limitKey] Optional limit key (inclusive) at
        which to end applying the callback.
      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} callback The callback to apply to the range.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} key The key.
        @param {String|Buffer} value The value.

  ###

  forRange: ->

    args = Array.prototype.slice.call arguments

    # required callback
    callback = args.pop()
    throw new Error 'Missing callback' unless callback

    # optional options
    options = args.pop() if typeof args[args.length - 1] is 'object'

    # optional keys
    if args.length is 2
      [ startKey, limitKey ] = args
    else
      startKey = args[0]

    limit = limitKey.toString 'binary' if limitKey

    next = (err) =>
      return callback err if err
      @current options, (err, key, val) =>
        return callback err if err
        if key
          callback null, key, val
          @next next if not limit or limit isnt key.toString 'binary'

    if startKey
      @seek startKey, next
    else
      @first next


  ###

      True if the iterator is positioned at a valid key.

  ###

  valid: ->
    @self.valid()


  ###

      Position the iterator at a key.

      @param {String} key The key at which to position the iterator.
      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  seek: (key, callback = noop) ->
    key = new Buffer key unless Buffer.isBuffer key
    @self.seek key, callback


  ###

      Synchronous version of `Iterator.seek()`.

  ###

  seekSync: (key) ->
    key = new Buffer key unless Buffer.isBuffer key
    @self.seek key


  ###

      Position the iterator at the first key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  first: (callback = noop) ->
    @self.first callback


  ###

      Synchronous version of `Iterator.first()`.

  ###

  firstSync: ->
    @self.first()


  ###

      Position the iterator at the last key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  last: (callback = noop) ->
    @self.last callback


  ###

      Synchronous version of `Iterator.last()`.

  ###

  lastSync: ->
    @self.last()


  ###

      Advance the iterator to the next key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  next: (callback = noop) ->
    @self.next callback


  ###

      Synchronous version of `Iterator.next()`.

  ###

  nextSync: ->
    @self.next()


  ###

      Advance the iterator to the previous key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  prev: (callback = noop) ->
    @self.prev callback


  ###

      Synchronous version of `Iterator.prev()`.

  ###

  prevSync: ->
    @self.prev()


  ###

      Get the key at the current iterator position.

      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} [callback] Optional callback. If not given, returns
        the key synchronously.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} key The key if successful.

  ###

  key: (options = {}, callback) ->

    # optional options
    if typeof options is 'function'
      callback = options
      options = {}

    if callback

      # async
      @self.key (err, key) ->
        key = key.toString 'utf8' unless err or options.as_buffer
        callback err, key

    else

      # sync
      key = @self.key()
      key = key.toString 'utf8' if key and not options.as_buffer
      key


  ###

      Get the value at the current iterator position.

      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} [callback] Optional callback. If not given, returns
        the value synchronously.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} value The value if successful.

  ###

  value: (options = {}, callback) ->

    # optional options
    if typeof options is 'function'
      callback = options
      options = {}

    if callback

      # async
      @self.value (err, val) ->
        val = val.toString 'utf8' unless err or options.as_buffer
        callback err, val

    else

      # sync
      val = @self.value()
      val = val.toString 'utf8' if val and not options.as_buffer
      val


  ###

      Get the key and value at the current iterator position.

      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} [callback] Optional callback. If not given, returns
        the key synchronously.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} key The key if successful.
        @param {String|Buffer} value The value if successful.

  ###

  current: (options = {}, callback) ->

    # optional options
    if typeof options is 'function'
      callback = options
      options = {}

    if callback

      # async
      @self.current (err, kv) ->
        if kv
          [ key, val ] = kv
          unless err or options.as_buffer
            key = key.toString 'utf8'
            val = val.toString 'utf8'
        callback err, key, val

    else

      # sync
      if kv = @self.current()
        [ key, val ] = kv
        unless options.as_buffer
          val = val.toString 'utf8'
          key = key.toString 'utf8'
      [ key, val ]
