Using a banmaster server
========================

Compile the banmaster server:

  bam banmaster

The banmaster server will execute the bans.cfg file. Add your bans to it as
lines in the form:

  ban $ADDRESS $REASON

For example:

  ban 192.168.0.130 Not welcome on this server

Run the banmaster server:

  ./banmaster

The bans.cfg file is automatically reread every 5 minutes.

For your servers there is a banmasters.cfg that looks like this:

  clear_banmasters
  add_banmaster 127.0.0.1

You can add multiple banmaster servers to it.
