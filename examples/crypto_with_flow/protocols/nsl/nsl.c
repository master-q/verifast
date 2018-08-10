#include "nsl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//@ #include "quantifiers.gh"

#define SERVER_PORT 121212

int random_stub_nsl(void *havege_state, char *output, size_t len)
  //@ requires PRG_PRECONDITION(havege_state_initialized, havege_state);
  //@ ensures PRG_POSTCONDITION(havege_state_initialized, havege_state);
{
  return havege_random(havege_state, output, len);
}

/*@

#define GENERAL_PRE(PRINCIPAL) \
  [_]public_invar(nsl_pub) &*& \
  [_]decryption_key_classifier(nsl_public_key) &*& \
  principal(PRINCIPAL, ?p_id) &*& \
  integer(socket, ?s) &*& \
    net_status(s, nil, ?port, connected) &*& \
  havege_state_initialized(havege_state) \

#define GENERAL_POST(PRINCIPAL) \
  principal(PRINCIPAL, _) &*& \
  integer(socket, s) &*& \
    net_status(s, nil, port, connected) &*& \
  havege_state_initialized(havege_state) \

@*/

/*@
predicate sender_inter(list<char> r_nonce_cs, cryptogram r_nonce_cg) = true;

#define SENDER_INTER \
  sender_inter(?r_nonce_cs, ?r_nonce_cg) &*& \
  ( \
    col || bad(sender) || bad(recvr) ? \
      chars(r_nonce, NONCE_SIZE, r_nonce_cs) \
    : \
      cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg) &*& \
      r_nonce_cg == cg_nonce(recvr, _) &*& \
      cg_info(r_nonce_cg) == int_pair(2, int_pair(sender, int_pair(sender, \
                                         int_pair(recvr, r_id)))) \
  )
@*/

void sender_msg1(int* socket, havege_state* havege_state, pk_context* r_context,
                 int sender, char* s_nonce)
  /*@ requires GENERAL_PRE(sender) &*&
               pk_context_with_key(r_context, pk_public,
                                   ?receiver, ?r_id, 8 * KEY_SIZE) &*&
               cryptogram(s_nonce, NONCE_SIZE, ?s_nonce_cs, ?s_nonce_cg) &*&
                 s_nonce_cg == cg_nonce(sender, _) &*&
                 cg_info(s_nonce_cg) == int_pair(1, int_pair(receiver, r_id)); @*/
  /*@ ensures  GENERAL_POST(sender) &*&
               pk_context_with_key(r_context, pk_public,
                                   receiver, r_id, 8 * KEY_SIZE) &*&
               cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg); @*/
{
  //@ open principal(sender, _);
  unsigned int size;
  char message[MSG1_SIZE];
  char encrypted[KEY_SIZE];

  // Construct the message
  write_identifier(message, sender);
  //@ assert chars(message, ID_SIZE, identifier(sender));
  //@ public_chars(message, ID_SIZE);
  //@ chars_to_secret_crypto_chars(message, ID_SIZE);

  //@ open cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
  memcpy((void*) message + ID_SIZE, s_nonce, NONCE_SIZE);
  //@ crypto_chars_join(message);
  //@ list<char> msg1 = append(identifier(sender), s_nonce_cs);
  //@ assert crypto_chars(secret, message, MSG1_SIZE, msg1);

   // Encrypt the message
  /*@ produce_function_pointer_chunk random_function(random_stub_nsl)
                    (havege_state_initialized)(state, out, len) { call(); } @*/
  //@ close random_state_predicate(havege_state_initialized);
  if (pk_encrypt(r_context, message, MSG1_SIZE, encrypted, &size,
                  KEY_SIZE, random_stub_nsl, havege_state) != 0)
    abort();
  //@ assert u_integer(&size, ?size_val);
  //@ assert cryptogram(encrypted, size_val, ?cs_enc, ?cg_enc);

  // Proof the message is public
  //@ take_append(ID_SIZE, identifier(sender), s_nonce_cs);
  //@ drop_append(ID_SIZE, identifier(sender), s_nonce_cs);
  //@ close nsl_pub_msg_sender(sender);
  //@ leak nsl_pub_msg_sender(sender);
  /*@ if (col || bad(sender) || bad(receiver))
      {
        assert crypto_chars(secret, message, MSG1_SIZE, msg1);
        crypto_chars_split(message, ID_SIZE);
        public_crypto_chars(message, ID_SIZE);
        assert chars(message, ID_SIZE, identifier(sender));
        close nsl_pub(s_nonce_cg);
        leak nsl_pub(s_nonce_cg);
        close cryptogram((void*) message + ID_SIZE, NONCE_SIZE,
                          s_nonce_cs, s_nonce_cg);
        public_cryptogram((void*) message + ID_SIZE, s_nonce_cg);
        assert chars((void*) message + ID_SIZE, NONCE_SIZE, s_nonce_cs);
        chars_join(message);
        public_chars(message, MSG1_SIZE);
        chars_to_crypto_chars(message, MSG1_SIZE);
      }
      else
      {
        close nsl_pub_msg1(sender, receiver, s_nonce_cg);
      }
  @*/
  //@ close nsl_pub(cg_enc);
  //@ leak nsl_pub(cg_enc);
  //@ public_cryptogram(encrypted, cg_enc);

  // Send the message
  net_send(socket, encrypted, size);
  zeroize(message, MSG1_SIZE);
  //@ close cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
  //@ close principal(sender, _);
}

void sender_msg2(int* socket, havege_state* havege_state, pk_context* s_context,
                 int sender, int recvr, char* s_nonce, char* r_nonce)
  /*@ requires GENERAL_PRE(sender) &*&
               pk_context_with_key(s_context, pk_private,
                                   sender, ?s_id, 8 * KEY_SIZE)  &*&
               pk_context_with_key(?r_context, pk_public,
                                   recvr, ?r_id, 8 * KEY_SIZE)  &*&
               cryptogram(s_nonce, NONCE_SIZE, ?s_nonce_cs, ?s_nonce_cg) &*&
                 s_nonce_cg == cg_nonce(sender, _) &*&
                 cg_info(s_nonce_cg) == int_pair(1, int_pair(recvr, r_id)) &*&
               chars(r_nonce, NONCE_SIZE, _); @*/
  /*@ ensures  GENERAL_POST(sender) &*&
               pk_context_with_key(s_context, pk_private,
                                   sender, s_id, 8 * KEY_SIZE) &*&
               pk_context_with_key(r_context, pk_public,
                                   recvr, r_id, 8 * KEY_SIZE)  &*&
               cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg) &*&
               SENDER_INTER; @*/
{
  //@ open principal(sender, _);
  unsigned int size;
  char message[MSG2_SIZE];
  char encrypted[KEY_SIZE];

  // Receive the message
  net_recv(socket, encrypted, KEY_SIZE);
  //@ assert chars(encrypted, KEY_SIZE, ?enc_cs);
  //@ interpret_asym_encrypted(encrypted, KEY_SIZE);

  // Decrypt the message
  /*@ produce_function_pointer_chunk random_function(random_stub_nsl)
                    (havege_state_initialized)(state, out, len) { call(); } @*/
  //@ close random_state_predicate(havege_state_initialized);
  //@ structure st = known_value(0, identifier(recvr));
  //@ close decryption_pre(false, false, sender, st, enc_cs);
  if (pk_decrypt(s_context, encrypted, KEY_SIZE, message,
                  &size, MSG2_SIZE, random_stub_nsl, havege_state) != 0)
    abort();
  if (size != MSG2_SIZE)
    abort();
  //@ assert cryptogram(encrypted, KEY_SIZE, enc_cs, ?enc_cg);
  //@ public_cryptogram(encrypted, enc_cg);
  //@ assert enc_cg == cg_asym_encrypted(?p, ?c, ?pay, ?ent);
  //@ assert u_integer(&size, MSG2_SIZE);
  /*@ open decryption_post(false, ?garbage, sender,
                           st, sender, s_id, ?dec_cs); @*/
  //@ assert crypto_chars(?kind, message, MSG2_SIZE, dec_cs);
  //@ crypto_chars_split(message, ID_SIZE);
  //@ crypto_chars_split((void*) message + ID_SIZE, NONCE_SIZE);                           
  //@ assert crypto_chars(kind, message, ID_SIZE, ?id_cs);
  //@ assert crypto_chars(kind, (void*) message + ID_SIZE, NONCE_SIZE, ?s_cs);
  /*@ assert crypto_chars(kind, (void*) message + ID_SIZE + NONCE_SIZE,
                          NONCE_SIZE, ?r_cs); @*/
  //@ append_assoc(id_cs, s_cs, r_cs);
  //@ take_append(ID_SIZE, id_cs, append(s_cs, r_cs));
  //@ drop_append(ID_SIZE, id_cs, append(s_cs, r_cs));
  //@ take_append(NONCE_SIZE, s_cs, r_cs);
  //@ drop_append(NONCE_SIZE, s_cs, r_cs);
  
  // Interpret the message
  //@ open [_]nsl_pub(enc_cg);
  //@ assert [_]nsl_pub_msg_sender(?recvr2);
  /*@ if (col || garbage)
      {
        chars_to_crypto_chars(message, ID_SIZE);
        chars_to_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
        chars_to_crypto_chars((void*) message + ID_SIZE + NONCE_SIZE, NONCE_SIZE);
      }
      else
      {
        if (bad(sender) || bad(recvr2))
        {
          assert [_]public_generated(nsl_pub)(dec_cs);
          public_generated_split(nsl_pub, dec_cs, ID_SIZE);
          public_generated_split(nsl_pub, append(s_cs, r_cs), NONCE_SIZE);
          public_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
          chars_to_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
        }
        else
        {
          assert [_]nsl_pub_msg2(sender, recvr2, ?s_nonce_cg2, ?s_nonce_cs2, 
                                 ?r_nonce_cg, _, _, _, ?pub_ns);
          take_append(ID_SIZE, identifier(recvr2), 
                      append(s_nonce_cs2, chars_for_cg(r_nonce_cg)));
          drop_append(ID_SIZE, identifier(recvr2), 
                      append(s_nonce_cs2, chars_for_cg(r_nonce_cg)));
          take_append(NONCE_SIZE, s_nonce_cs2, chars_for_cg(r_nonce_cg));
          drop_append(NONCE_SIZE, s_nonce_cs2, chars_for_cg(r_nonce_cg));
          if (pub_ns)
          {
            public_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
            chars_to_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
          }
          else
          {
            close memcmp_secret((void*) message + ID_SIZE, NONCE_SIZE, 
                                chars_for_cg(s_nonce_cg2), s_nonce_cg2);
          }
        }
        public_crypto_chars(message, ID_SIZE);
      }
  @*/
  //@ close check_identifier_ghost_args(false, garbage, sender, s_id, append(s_cs, r_cs));
  check_identifier(message, recvr);                                                   
  //@ open cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
  //@ close memcmp_secret(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
  if (memcmp((void*) message + ID_SIZE, s_nonce, NONCE_SIZE) != 0) abort();
  memcpy(r_nonce, (void*) message + ID_SIZE + NONCE_SIZE, NONCE_SIZE);
  //@ assert id_cs == identifier(recvr);
  //@ assert s_cs == s_nonce_cs;
  //@ cryptogram r_nonce_cg = chars_for_cg_sur(r_cs, tag_nonce);
  //@ close sender_inter(r_cs, r_nonce_cg);
  /*@ if (!col && !garbage)
      {
        if (bad(sender) || bad(recvr2))
        {
          assert [_]public_generated(nsl_pub)(dec_cs);
          public_generated_split(nsl_pub, dec_cs, ID_SIZE);
          public_generated_split(nsl_pub, append(s_nonce_cs, r_cs), NONCE_SIZE);
          public_crypto_chars(r_nonce, NONCE_SIZE);
          public_crypto_chars_extract(s_nonce, s_nonce_cg);
          open [_]nsl_pub(s_nonce_cg);
          chars_to_secret_crypto_chars(s_nonce, NONCE_SIZE);
          chars_to_secret_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
        }
        else
        {
          assert dec_cs == pay;
          assert sender == p;
          assert s_id == c;
          assert [_]nsl_pub_msg2(sender, recvr2, ?s_nonce_cg2, ?s_nonce_cs2,
                                 ?r_nonce_cg2, ?p_inst, ?p2, ?c2, ?pub_ns);
          assert identifier(recvr) == identifier(recvr2);
          equal_identifiers(recvr, recvr2);
          if (pub_ns)
          {
            public_crypto_chars_extract(s_nonce, s_nonce_cg);
            open [_]nsl_pub(s_nonce_cg);
            assert false;
          }
          chars_for_cg_inj(r_nonce_cg2, r_nonce_cg);
          close cryptogram(r_nonce, NONCE_SIZE, r_cs, r_nonce_cg2);
          chars_for_cg_inj(s_nonce_cg, s_nonce_cg2);
          assert c2 == r_id;
        }
      }
  @*/
  //@ close cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
  //@ crypto_chars_join((void*) message + ID_SIZE);
  zeroize((void*) message + ID_SIZE, 2 * NONCE_SIZE);
  //@ crypto_chars_to_chars(message, ID_SIZE);
  //@ chars_join(message);
  //@ close principal(sender, _);
}

void sender_msg3(int* socket, havege_state* havege_state, pk_context* r_context,
                 int sender, char* r_nonce)
  /*@ requires GENERAL_PRE(sender) &*&
               pk_context_with_key(r_context, pk_public,
                                   ?recvr, ?r_id, 8 * KEY_SIZE) &*&
               SENDER_INTER; @*/
  /*@ ensures  GENERAL_POST(sender) &*&
               pk_context_with_key(r_context, pk_public,
                                   recvr, r_id, 8 * KEY_SIZE) &*&
               col || bad(sender) || bad(recvr) ?
                 chars(r_nonce, NONCE_SIZE, r_nonce_cs)
               :
                 cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg); @*/
{
  //@ open principal(sender, _);
  unsigned int size;
  char message[MSG3_SIZE];
  char encrypted[KEY_SIZE];

  // Construct the message
  //@ open sender_inter(r_nonce_cs, r_nonce_cg);
  //@ bool condition = col || bad(sender) || bad(recvr);
  /*@ if (!condition)
        open cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
      else
      {
        public_chars(r_nonce, NONCE_SIZE);
        chars_to_secret_crypto_chars(r_nonce, NONCE_SIZE);
      }
  @*/
  memcpy((void*) message, r_nonce, NONCE_SIZE);
  /*@ if (!condition)
        close cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
      else
        public_crypto_chars(r_nonce, NONCE_SIZE);
  @*/

  // Encrypt the message
  /*@ produce_function_pointer_chunk random_function(random_stub_nsl)
                    (havege_state_initialized)(state, out, len) { call(); } @*/
  //@ close random_state_predicate(havege_state_initialized);
  if (pk_encrypt(r_context, message, MSG3_SIZE, encrypted, &size,
                 KEY_SIZE, random_stub_nsl, havege_state) != 0)
    abort();
  //@ assert u_integer(&size, ?size_val);
  //@ assert cryptogram(encrypted, size_val, ?cs_enc, ?cg_enc);

  // Proof the message is public
  //@ close nsl_pub_msg_sender(sender);
  //@ leak nsl_pub_msg_sender(sender);
  //@ if (!condition) close nsl_pub_msg3(sender, recvr, r_nonce_cg);
  //@ close nsl_pub(cg_enc);
  //@ leak nsl_pub(cg_enc);
  //@ public_cryptogram(encrypted, cg_enc);

  // Send the message
  net_send(socket, encrypted, size);
  zeroize(message, MSG3_SIZE);
  //@ close principal(sender, _);
}

void sender(int sender, int receiver,
            char *s_priv_key, char *r_pub_key,
            char *s_nonce, char *r_nonce)
/*@ requires [_]public_invar(nsl_pub) &*&
             [_]decryption_key_classifier(nsl_public_key) &*&
             principal(sender, _) &*&
             [?f1]cryptogram(s_priv_key, 8 * KEY_SIZE,
                             ?s_priv_key_cs, ?s_priv_key_cg) &*&
               s_priv_key_cg == cg_private_key(sender, ?s_id) &*&
             [?f2]cryptogram(r_pub_key, 8 * KEY_SIZE,
                             ?r_pub_key_cs, ?r_pub_key_cg) &*&
               r_pub_key_cg == cg_public_key(receiver, ?r_id) &*&
             chars(s_nonce, NONCE_SIZE, _) &*&
             chars(r_nonce, NONCE_SIZE, _); @*/
/*@ ensures  principal(sender, _) &*&
             [f1]cryptogram(s_priv_key, 8 * KEY_SIZE,
                            s_priv_key_cs, s_priv_key_cg) &*&
             [f2]cryptogram(r_pub_key, 8 * KEY_SIZE,
                            r_pub_key_cs, r_pub_key_cg) &*&
             cryptogram(s_nonce, NONCE_SIZE, ?s_nonce_cs, ?s_nonce_cg) &*&
               s_nonce_cg == cg_nonce(sender, _) &*&
               cg_info(s_nonce_cg) == int_pair(1, int_pair(receiver, r_id)) &*&
             col || bad(sender) || bad(receiver) ?
               chars(r_nonce, NONCE_SIZE, _)
             :
               cryptogram(r_nonce, NONCE_SIZE, _, ?r_nonce_cg) &*&
               r_nonce_cg == cg_nonce(receiver, _) &*&
               cg_info(r_nonce_cg) == int_pair(2, int_pair(sender, int_pair(sender,
                                                  int_pair(receiver, r_id)))); @*/
{
  int socket;
  pk_context s_context;
  pk_context r_context;
  havege_state havege_state;

  net_usleep(20000);
  if(net_connect(&socket, NULL, SERVER_PORT) != 0)
    abort();
  if(net_set_block(socket) != 0)
    abort();

  //@ close pk_context(&s_context);
  pk_init(&s_context);
  if (pk_parse_key(&s_context, s_priv_key,
                   (unsigned int) 8 * KEY_SIZE, NULL, 0) != 0)
    abort();
  //@ close pk_context(&r_context);
  pk_init(&r_context);
  if (pk_parse_public_key(&r_context, r_pub_key,
                          (unsigned int) 8 * KEY_SIZE) != 0)
    abort();

  // Generate NA
  //@ open principal(sender, _);
  //@ close havege_state(&havege_state);
  havege_init(&havege_state);
  //@ close random_request(sender, int_pair(1, int_pair(receiver, r_id)), false);
  if (havege_random(&havege_state, s_nonce, NONCE_SIZE) != 0) abort();
  //@ assert cryptogram(s_nonce, NONCE_SIZE, ?cs_s_nonce, ?cg_s_nonce);
  //@ close principal(sender, _);

  sender_msg1(&socket, &havege_state, &r_context, sender, s_nonce);
  sender_msg2(&socket, &havege_state, &s_context, sender, receiver,
              s_nonce, r_nonce);
  sender_msg3(&socket, &havege_state, &r_context, sender, r_nonce);
  havege_free(&havege_state);
  //@ open havege_state(&havege_state);

  //@ pk_release_context_with_key(&s_context);
  pk_free(&s_context);
  //@ open pk_context(&s_context);
  //@ pk_release_context_with_key(&r_context);
  pk_free(&r_context);
  //@ open pk_context(&r_context);

  net_close(socket);
}

/*@
predicate receiver_inter(int p_original, int c_original, int p_instigator,
                         list<char> s_nonce_cs, cryptogram s_nonce_cg) = true;

#define RECEIVER_INTER \
  ( \
    receiver_inter(?p_orig, ?c_orig, ?p_inst, ?s_nonce_cs, ?s_nonce_cg) &*& \
    col || bad(p_inst) || bad(receiver) ? \
      chars(s_nonce, NONCE_SIZE, s_nonce_cs) \
    : \
      receiver == p_orig && r_id == c_orig &*& \
      cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg) &*& \
      sender == p_inst &*& s_nonce_cg == cg_nonce(sender, _) &*& \
      cg_info(s_nonce_cg) == int_pair(1, int_pair(receiver, r_id)) \
  )
@*/

void receiver_msg1(int* socket, havege_state* havege_state,
                   pk_context* r_context, int sender, int receiver,
                   char* s_nonce)
  /*@ requires GENERAL_PRE(receiver) &*&
               pk_context_with_key(r_context, pk_private,
                                   receiver, ?r_id, 8 * KEY_SIZE)  &*&
               chars(s_nonce, NONCE_SIZE, _); @*/
  /*@ ensures  GENERAL_POST(receiver) &*&
               pk_context_with_key(r_context, pk_private,
                                   receiver, r_id, 8 * KEY_SIZE) &*&
               RECEIVER_INTER; @*/
{
  //@ open principal(receiver, _);
  unsigned int size;
  char message[MSG1_SIZE];
  char encrypted[KEY_SIZE];

  // Receive the message
  net_recv(socket, encrypted, KEY_SIZE);
  //@ assert chars(encrypted, KEY_SIZE, ?enc_cs);
  //@ interpret_asym_encrypted(encrypted, KEY_SIZE);
  //@ assert cryptogram(encrypted, KEY_SIZE, enc_cs, ?enc_cg);
  //@ assert enc_cg == cg_asym_encrypted(?p, ?c, ?pay, ?ent);
  //@ public_cryptogram_extract(encrypted);

  // Decrypt the message
  /*@ produce_function_pointer_chunk random_function(random_stub_nsl)
                    (havege_state_initialized)(state, out, len) { call(); } @*/
  //@ close random_state_predicate(havege_state_initialized);
  //@ structure st = known_value(0, identifier(sender));
  //@ close decryption_pre(false, false, receiver, st, enc_cs);
  if (pk_decrypt(r_context, encrypted, KEY_SIZE, message,
                  &size, MSG1_SIZE, random_stub_nsl, havege_state) != 0)
    abort();
  if (size != MSG1_SIZE)
    abort();
  //@ assert u_integer(&size, MSG1_SIZE);
  //@ assert crypto_chars(?kind, message, MSG1_SIZE, ?dec_cs);
  /*@ open decryption_post(false, ?garbage, receiver,
                           st, receiver, r_id, dec_cs); @*/
  //@ crypto_chars_split(message, ID_SIZE);
  //@ assert crypto_chars(kind, message, ID_SIZE, ?id_cs);
  //@ assert crypto_chars(kind, (void*) message + ID_SIZE, NONCE_SIZE, ?s_nonce_cs);
  //@ assert dec_cs == append(id_cs, s_nonce_cs);

  //@ open [_]nsl_pub(enc_cg);
  //@ assert [_]nsl_pub_msg_sender(?p_instigator2);
  /*@ if (col || garbage)
      {
        chars_to_crypto_chars(message, ID_SIZE);
      }
      else
      {
        if (bad(receiver) || bad(p_instigator2))
        {
          assert [_]public_generated(nsl_pub)(dec_cs);
          public_generated_split(nsl_pub, dec_cs, ID_SIZE);
        }
        else
        {
          assert [_]nsl_pub_msg1(_, _, ?s_nonce_cg);
          take_append(ID_SIZE, identifier(p_instigator2), chars_for_cg(s_nonce_cg));
          drop_append(ID_SIZE, identifier(p_instigator2), chars_for_cg(s_nonce_cg));
        }
        public_crypto_chars(message, ID_SIZE);
      }
  @*/
  //@ close check_identifier_ghost_args(false, garbage, receiver, r_id, s_nonce_cs);
  check_identifier(message, sender);
  //@ assert id_cs == identifier(sender);
  memcpy(s_nonce, (void*) message + ID_SIZE, NONCE_SIZE);

  // Interpret message
  //@ cryptogram s_nonce_cg;
  //@ int p_inst;
  /*@ if (!col && !garbage)
      {
        p_inst = p_instigator2;
        if (bad(p_inst) || bad(receiver))
        {
          public_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
          public_crypto_chars(s_nonce, NONCE_SIZE);
          chars_to_crypto_chars(message, ID_SIZE);
          chars_to_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
        }
        else
        {
          assert [_]nsl_pub_msg1(p_inst, receiver, ?s_nonce_cg2);
          s_nonce_cg = s_nonce_cg2;
          assert s_nonce_cg == cg_nonce(p_inst, _);
          s_nonce_cs = chars_for_cg(s_nonce_cg);
          assert dec_cs == append(identifier(p_inst), s_nonce_cs);
          take_append(ID_SIZE, identifier(p_inst), s_nonce_cs);
          drop_append(ID_SIZE, identifier(p_inst), s_nonce_cs);
          assert identifier(p_inst) == identifier(sender);
          equal_identifiers(p_inst, sender);
          close cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
        }
      }
  @*/
  //@ close receiver_inter(p, c, p_inst, s_nonce_cs, s_nonce_cg);
  //@ crypto_chars_to_chars(message, ID_SIZE);
  zeroize((void*) message + ID_SIZE, NONCE_SIZE);
  //@ public_cryptogram(encrypted, enc_cg);
  //@ close principal(receiver, _);
}

void receiver_msg2(int* socket, havege_state* havege_state,
                   pk_context* s_context, int receiver,
                   char* s_nonce, char* r_nonce)
  /*@ requires GENERAL_PRE(receiver) &*&
               pk_context_with_key(?r_context, pk_private,
                                   receiver, ?r_id, 8 * KEY_SIZE)  &*&
               pk_context_with_key(s_context, pk_public,
                                   ?sender, ?s_id, 8 * KEY_SIZE) &*&
               RECEIVER_INTER &*&
               cryptogram(r_nonce, NONCE_SIZE, ?r_nonce_cs, ?r_nonce_cg) &*&
                 r_nonce_cg == cg_nonce(receiver, _) &*&
                 cg_info(r_nonce_cg) == int_pair(2, int_pair(sender,
                               int_pair(p_inst, int_pair(p_orig, c_orig)))); @*/
  /*@ ensures  GENERAL_POST(receiver) &*&
               pk_context_with_key(r_context, pk_private,
                                   receiver, r_id, 8 * KEY_SIZE)  &*&
               pk_context_with_key(s_context, pk_public,
                                   sender, s_id, 8 * KEY_SIZE) &*&
               (
                  col || bad(p_inst) || bad(receiver) ?
                    chars(s_nonce, NONCE_SIZE, s_nonce_cs)
                  :
                    cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg)
               ) &*&
               cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg); @*/
{
  //@ open principal(receiver, _);
  unsigned int size;
  char message[MSG2_SIZE];
  char encrypted[KEY_SIZE];
  //@ open receiver_inter(p_orig, c_orig, p_inst, s_nonce_cs, s_nonce_cg);

  // Construct the message
  write_identifier(message, receiver);
  //@ assert chars(message, ID_SIZE, identifier(receiver));
  //@ public_chars(message, ID_SIZE);
  //@ chars_to_secret_crypto_chars(message, ID_SIZE);
  //@ bool condition =  col || bad(p_inst) || bad(receiver);
  /*@ if (!condition)
        open cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
      else
      {
        public_chars(s_nonce, NONCE_SIZE);
        chars_to_secret_crypto_chars(s_nonce, NONCE_SIZE);
      }
  @*/
  //@ open cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
  memcpy((void*) message + ID_SIZE, s_nonce, NONCE_SIZE);
  memcpy((void*) message + ID_SIZE + NONCE_SIZE, r_nonce, NONCE_SIZE);
  //@ crypto_chars_join(message);
  //@ crypto_chars_join(message);
  /*@ list<char> msg2 = append(identifier(receiver),
                               append(s_nonce_cs, r_nonce_cs)); @*/
  //@ append_assoc(identifier(receiver), s_nonce_cs, r_nonce_cs);
  //@ assert crypto_chars(secret, message, MSG2_SIZE, msg2);

  // Encrypt the message
  /*@ produce_function_pointer_chunk random_function(random_stub_nsl)
                    (havege_state_initialized)(state, out, len) { call(); } @*/
  //@ close random_state_predicate(havege_state_initialized);
  if (pk_encrypt(s_context, message, MSG2_SIZE, encrypted, &size,
                  KEY_SIZE, random_stub_nsl, havege_state) != 0)
    abort();
  //@ assert u_integer(&size, ?size_val);
  //@ assert cryptogram(encrypted, size_val, ?cs_enc, ?cg_enc);
  //@ take_append(ID_SIZE, identifier(receiver), append(s_nonce_cs, r_nonce_cs));
  //@ drop_append(ID_SIZE, identifier(receiver), append(s_nonce_cs, r_nonce_cs));
  //@ take_append(NONCE_SIZE, s_nonce_cs, r_nonce_cs);
  //@ drop_append(NONCE_SIZE, s_nonce_cs, r_nonce_cs);

  // Proof the message is public
  /*@ if (col || bad(sender) || bad(receiver))
      {
        assert crypto_chars(secret, message, MSG2_SIZE, msg2);
        crypto_chars_split(message, ID_SIZE);
        crypto_chars_split((void*) message + ID_SIZE, NONCE_SIZE);
        public_crypto_chars((void*) message, ID_SIZE);
        public_crypto_chars((void*) message + ID_SIZE, NONCE_SIZE);
        close nsl_pub(r_nonce_cg);
        leak nsl_pub(r_nonce_cg);
        close cryptogram((void*) message + ID_SIZE + NONCE_SIZE, NONCE_SIZE,
                         r_nonce_cs, r_nonce_cg);
        public_cryptogram((void*) message + ID_SIZE + NONCE_SIZE, r_nonce_cg);
        chars_join(message);
        chars_join(message);
        public_chars(message, MSG2_SIZE);
        chars_to_secret_crypto_chars(message, MSG2_SIZE);
      }
      else
      {
        close nsl_pub_msg2(sender, receiver, s_nonce_cg, s_nonce_cs, r_nonce_cg,
                           p_inst, p_orig, c_orig, condition);
      }
  @*/
  //@ close nsl_pub_msg_sender(receiver);
  //@ leak nsl_pub_msg_sender(receiver);
  //@ close nsl_pub(cg_enc);
  //@ leak nsl_pub(cg_enc);
  //@ public_cryptogram(encrypted, cg_enc);

  // Send the message
  net_send(socket, encrypted, size);
  zeroize(message, MSG2_SIZE);

  /*@ if (!condition)
        close cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg);
      else
        public_crypto_chars(s_nonce, NONCE_SIZE);
  @*/
  //@ close cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
  //@ close principal(receiver, _);
}

void receiver_msg3(int* socket, havege_state* havege_state,
                   pk_context* r_context, int sender, int receiver,
                   char* s_nonce, char* r_nonce)
  /*@ requires GENERAL_PRE(receiver) &*&
               pk_context_with_key(r_context, pk_private,
                                   receiver, ?r_id, 8 * KEY_SIZE)  &*&
               RECEIVER_INTER &*&
               cryptogram(r_nonce, NONCE_SIZE, ?r_nonce_cs, ?r_nonce_cg) &*&
                 r_nonce_cg == cg_nonce(receiver, _) &*&
                 cg_info(r_nonce_cg) == int_pair(2, int_pair(sender,
                               int_pair(p_inst, int_pair(p_orig, c_orig)))); @*/
  /*@ ensures  GENERAL_POST(receiver) &*&
               pk_context_with_key(r_context, pk_private,
                                   receiver, r_id, 8 * KEY_SIZE) &*&
               (
                 col || bad(p_inst) || bad(receiver) ?
                   chars(s_nonce, NONCE_SIZE, s_nonce_cs)
                 :
                   cryptogram(s_nonce, NONCE_SIZE, s_nonce_cs, s_nonce_cg)
               ) &*&
               cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg) &*&
               col || bad(sender) || bad(receiver) ||
                 (receiver == p_orig && r_id == c_orig && sender == p_inst); @*/
{
  //@ open principal(receiver, _);
  unsigned int size;
  char message[MSG3_SIZE];
  char encrypted[KEY_SIZE];
  //@ open receiver_inter(p_orig, c_orig, p_inst, s_nonce_cs, s_nonce_cg);

  // Receive the message
  net_recv(socket, encrypted, KEY_SIZE);
  //@ assert chars(encrypted, KEY_SIZE, ?enc_cs);
  //@ interpret_asym_encrypted(encrypted, KEY_SIZE);
  //@ assert cryptogram(encrypted, KEY_SIZE, enc_cs, ?enc_cg);
  //@ assert enc_cg == cg_asym_encrypted(?p, ?c, ?pay, ?ent);

  // Decrypt the message
  /*@ produce_function_pointer_chunk random_function(random_stub_nsl)
                    (havege_state_initialized)(state, out, len) { call(); } @*/
  //@ close random_state_predicate(havege_state_initialized);
  //@ structure st = known_value(0, r_nonce_cs);
  //@ close decryption_pre(false, false, receiver, st, enc_cs);
  if (pk_decrypt(r_context, encrypted, KEY_SIZE, message,
                  &size, MSG3_SIZE, random_stub_nsl, havege_state) != 0)
    abort();
  if (size != MSG3_SIZE)
    abort();
  //@ assert u_integer(&size, MSG3_SIZE);
  //@ assert crypto_chars(?kind, message, NONCE_SIZE, ?dec_cs);
  /*@ open decryption_post(false, ?garbage, receiver,
                           st, receiver, r_id, dec_cs); @*/

  // Interpret the message
  //@ open cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
  //@ open [_]nsl_pub(enc_cg);
  //@ assert [_]nsl_pub_msg_sender(?sender2);
  /*@ if (col || garbage)
      {
        chars_to_crypto_chars(message, NONCE_SIZE);
      }
      else
      {
        if (bad(receiver) || bad(sender2))
        {
          public_crypto_chars(message, NONCE_SIZE);
          chars_to_crypto_chars(message, NONCE_SIZE);
        }
        else
        {
          assert [_]nsl_pub_msg3(sender2, receiver, ?r_nonce_cg2);
          close memcmp_secret(message, NONCE_SIZE, dec_cs, r_nonce_cg2);
        }
      }
  @*/
  //@ close memcmp_secret(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
  if (memcmp((void*) message, r_nonce, NONCE_SIZE) != 0) abort();
  //@ assert dec_cs == r_nonce_cs;
  /*@ if (garbage)
      {
        close exists(pair(nil, nil));
        close has_structure(r_nonce_cs, st);
        leak has_structure(r_nonce_cs, st);
        decryption_garbage(message, NONCE_SIZE, st);
        chars_to_secret_crypto_chars(message, NONCE_SIZE);
      }
      else if (!col && !bad(sender) && !bad(receiver))
      {
        if (bad(receiver) || bad(sender2))
        {
          public_crypto_chars(r_nonce, NONCE_SIZE);
          public_chars_extract(r_nonce, r_nonce_cg);
          open [_]nsl_pub(r_nonce_cg);
          assert false;
        }
        else if (bad(p_inst))
        {
          assert [_]nsl_pub_msg3(sender2, receiver, ?r_nonce_cg2);
          int p1_1 = int_pair(p_orig, c_orig);
          int p1_2 = int_pair(p_inst, p1_1);
          int p1_3 = int_pair(sender, p1_2);
          int p2_1 = int_pair(receiver, r_id);
          int p2_2 = int_pair(sender2, p2_1);
          int p2_3 = int_pair(sender2, p2_2);
          assert cg_info(r_nonce_cg) == int_pair(2, p1_3);
          assert cg_info(r_nonce_cg2) == int_pair(2, p2_3);
          chars_for_cg_inj(r_nonce_cg, r_nonce_cg2);
          int_right_int_pair(2, p1_3);
          int_right_int_pair(2, p2_3);
          int_left_int_pair(sender, p1_2);
          int_left_int_pair(sender2, p2_2);
          int_right_int_pair(sender, p1_2);
          int_right_int_pair(sender2, p2_2);
          int_left_int_pair(p_inst, p1_1);
          assert false;
        }
      }
  @*/
  zeroize(message, MSG3_SIZE);
  //@ close cryptogram(r_nonce, NONCE_SIZE, r_nonce_cs, r_nonce_cg);
  //@ public_cryptogram(encrypted, enc_cg);
  //@ close principal(receiver, _);
}

void receiver(int sender, int receiver,
              char *s_pub_key, char *r_priv_key,
              char *s_nonce, char *r_nonce)
/*@ requires [_]public_invar(nsl_pub) &*&
             [_]decryption_key_classifier(nsl_public_key) &*&
             principal(receiver, _) &*&
             [?f1]cryptogram(s_pub_key, 8 * KEY_SIZE,
                             ?s_pub_key_cs, ?s_pub_key_cg) &*&
               s_pub_key_cg == cg_public_key(sender, ?s_id) &*&
             [?f2]cryptogram(r_priv_key, 8 * KEY_SIZE,
                             ?r_priv_key_cs, ?r_priv_key_cg) &*&
               r_priv_key_cg == cg_private_key(receiver, ?r_id) &*&
             chars(s_nonce, NONCE_SIZE, _) &*&
             chars(r_nonce, NONCE_SIZE, _); @*/
/*@ ensures  principal(receiver, _) &*&
             [f1]cryptogram(s_pub_key, 8 * KEY_SIZE,
                            s_pub_key_cs, s_pub_key_cg) &*&
             [f2]cryptogram(r_priv_key, 8 * KEY_SIZE,
                            r_priv_key_cs, r_priv_key_cg) &*&
             cryptogram(r_nonce, NONCE_SIZE, ?r_nonce_cs, ?r_nonce_cg) &*&
               r_nonce_cg == cg_nonce(receiver, _) &*&
             (
               col || bad(sender) || bad(receiver) ?
                 chars(s_nonce, NONCE_SIZE, _)
               :
                 cryptogram(s_nonce, NONCE_SIZE, ?s_nonce_cs, ?s_nonce_cg) &*&
                 s_nonce_cg == cg_nonce(sender, _) &*&
                 cg_info(s_nonce_cg) == int_pair(1, int_pair(receiver, r_id)) &*&
                 cg_info(r_nonce_cg) == int_pair(2, int_pair(sender, int_pair(sender,
                                                    int_pair(receiver, r_id))))
             ); @*/
{
  //@ open principal(receiver, _);
  int socket1;
  int socket2;
  pk_context s_context;
  pk_context r_context;
  havege_state havege_state;

  if(net_bind(&socket1, NULL, SERVER_PORT) != 0)
    abort();
  if(net_accept(socket1, &socket2, NULL) != 0)
    abort();
  if(net_set_block(socket2) != 0)
    abort();

  //@ close pk_context(&s_context);
  pk_init(&s_context);
  if (pk_parse_public_key(&s_context, s_pub_key,
                          (unsigned int) 8 * KEY_SIZE) != 0)
    abort();
  //@ close pk_context(&r_context);
  pk_init(&r_context);
  if (pk_parse_key(&r_context, r_priv_key,
                   (unsigned int) 8 * KEY_SIZE, NULL, 0) != 0)
    abort();

  // Generate NB
  //@ close havege_state(&havege_state);
  havege_init(&havege_state);

  //@ close principal(receiver, _);
  receiver_msg1(&socket2, &havege_state, &r_context, sender, receiver, s_nonce);
  //@ open principal(receiver, _);

  //@ assert receiver_inter(?p_orig, ?c_orig, ?p_inst, ?s_nonce_cs, ?s_nonce_cg);
  //@ int info = int_pair(sender, int_pair(p_inst, int_pair(p_orig, c_orig)));
  //@ close random_request(receiver, int_pair(2, info), false);
  if (havege_random(&havege_state, r_nonce, NONCE_SIZE) != 0) abort();
  //@ close principal(receiver, _);
  receiver_msg2(&socket2, &havege_state, &s_context, receiver,
                s_nonce, r_nonce);
  //@ close receiver_inter(p_orig, c_orig, p_inst, s_nonce_cs, s_nonce_cg);
  receiver_msg3(&socket2, &havege_state, &r_context, sender, receiver,
                s_nonce, r_nonce);

  havege_free(&havege_state);
  //@ open havege_state(&havege_state);

  //@ pk_release_context_with_key(&s_context);
  pk_free(&s_context);
  //@ open pk_context(&s_context);
  //@ pk_release_context_with_key(&r_context);
  pk_free(&r_context);
  //@ open pk_context(&r_context);

  net_close(socket2);
  net_close(socket1);
}