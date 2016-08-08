<?php

/** Form callback for notifications settings form */
function gb_maillist_settings_form($form, &$form_state) {
  $form = array(
    'gb_maillist_outgoing_disabled' => array(
      '#type' => 'checkbox',
      '#title' => t('Disable outgoing e-mail'),
      '#description' => t('If set the mailing lists will not send e-mail.'),
      '#default_value' => variable_get(
          'gb_maillist_outgoing_disabled', FALSE),
    ),
    'gb_maillist_incoming_disabled' => array(
      '#type' => 'checkbox',
      '#title' => t('Disable incoming e-mail'),
      '#description' => t('If set the mailing lists will not accept' .
          ' incoming e-mail.'),
      '#default_value' => variable_get(
          'gb_maillist_incoming_disabled', FALSE),
    ),
    'gb_maillist_send_copy_to_sender' => array(
      '#type' => 'checkbox',
      '#title' => t('Send copy of outgoing e-mail to the original sender'),
      '#description' => t('If set the mailing lists will send a copy of all' .
          ' outgoing e-mail to the original sender, whether posted in the' .
          ' boards or mailed in.'),
      '#default_value' => variable_get(
          'gb_maillist_send_copy_to_sender', FALSE),
    ),
    'gb_maillist_strict_spam_filtering' => array(
      '#type' => 'checkbox',
      '#title' => t('Strict SPAM filtering'),
      '#description' => t('If set SPAM filtering is more strict.' .
          ' Less SPAM will get through but there will be more' .
          ' false positives.'),
      '#default_value' => variable_get(
          'gb_maillist_strict_spam_filtering', TRUE),
    ),
  );

  // check out the state of the DKIM for this domain
  $domain = g3core_get_domain();
  if( strpos($domain, 'groupanizer.com') === FALSE ) {

    $dns = dns_get_record('sing3._domainkey.'.$domain, DNS_TXT);
    $existing_pub_key = '';
    if( isset($dns[0]['txt']) ) {
      $raw_key = substr($dns[0]['txt'],strpos($dns[0]['txt'], 'p=') + 2);
      // format that key into the familiar block format, apparently required!
      $existing_pub_key = '';
      $linesize = 64;
      do {
        $existing_pub_key .= substr($raw_key,0,$linesize) . "\n";
        $raw_key = substr($raw_key,$linesize);
      } while (strlen($raw_key) > 0);
      $existing_pub_key = "-----BEGIN PUBLIC KEY-----\n" .  $existing_pub_key .  "-----END PUBLIC KEY-----";

      $private_key = variable_get('gl_mail_dkim_private_key', '');
      $clear_in = 'asdf';
      $encrypted = '';
      $clear_out = '';
      if( !openssl_private_encrypt($clear_in, $encrypted, $private_key) ) {
        watchdog('gb_maillist', "Failed test encryption");
      }
      if( !openssl_public_decrypt($encrypted, $clear_out, $existing_pub_key) ) {
        watchdog('gb_maillist', "Failed test decryption");
      }
      if( $clear_in == $clear_out ) {
        $status = 'status';
        $msg = t("Great news - your private key fits with the public key in the @domain domain!", array('@domain'=>$domain));
      } else {
        $status = 'warning';
        $msg = t("Unfortunate - your private key does not seem to fit with the public key in the @domain domain.", array('@domain'=>$domain));
      }

    } else {
      $status = 'warning';
      $msg = t("Right now there is no DKIM record in the @domain domain.", array('@domain'=>$domain));
    }

    $form['dkim'] = array(
      '#type' => 'fieldset',
      '#title' => 'Mail authentication (DKIM)',
      '#description' => t("DKIM is a way of 'authenticating' your mail, or proving that it is really coming from you, and not from a spammer.  Authenticating your outgoing mail may help it get accepted by more ISPs. We don't have access to your domain host, so unfortunately we can't do this for you!"),
      '#prefix' => '<div id="replace_this">',
      '#suffix' => '</div>',
    );

    $form['dkim']['status'] = array(
      '#type' => 'markup',
      '#markup' => $msg,
      '#prefix' => '<div class="messages ' . $status . '">',
      '#suffix' => '</div>',
    );

    $form['dkim']['instructions'] = array(
      '#type' => 'markup',
      '#markup' =>
        t("<p>Please follow these steps:</p><ol>") .
        t("<li>Generate your keys by clicking the button (or paste them in if you already have some you would rather use)</li>") .
        t("<li>Save this page, so the fields at the bottom of the form are populated</li>") .
        t("<li>Contact your domain host and get instructions for adding a TXT record</li>") .
        t("<li>Copy the TXT record name and TXT record value from below, and paste them into the correct place in the interface provided by your domain host</li>") .
        t("<li>Within 24 hours, your domain will propagate and DKIM will be operational</li>"),
    );

    $form['dkim']['gl_mail_dkim_public_key'] = array(
      '#type' => 'textarea',
      '#title' => 'Public key',
      '#description' => 'Paste your public key here, or generate a key pair with the "Generate new keys" button below.',
      '#default_value' => variable_get('gl_mail_dkim_public_key', 'Not yet generated'),
    );
    $form['dkim']['gl_mail_dkim_private_key'] = array(
      '#type' => 'textarea',
      '#title' => 'Private key',
      '#description' => 'If you generated keys, you won\'t need to do anything with this. If you are using keys you generated yourself, paste your private key in here.',
      '#default_value' => variable_get('gl_mail_dkim_private_key', 'Not yet generated'),
    );

    $form['dkim']['generate'] = array(
      '#type' => 'button',
      '#name' => 'dkim-button',
      '#value' => 'Generate new keys',
      '#ajax' => array(
        'callback' => 'gb_maillist_generate_keys',
        'wrapper' =>  'replace_this',
        'method' => 'replace',
        'effect' => 'fade',
      ),
    );

     $form['dkim']['gl_mail_record_name'] = array(
      '#title' => 'DKIM TXT record name)',
      '#type' => 'textfield',
      '#description' => 'Cut and paste this into the TXT record name field at your DNS host',
      '#default_value' => 'sing3._domainkey',
      '#disabled' => TRUE,
    );

    $txtVal = variable_get('gl_mail_dkim_public_key', 'Please generate or paste-in the public key');
    $txtVal = str_replace('-----BEGIN PUBLIC KEY-----','',$txtVal);
    $txtVal = str_replace('-----END PUBLIC KEY-----','',$txtVal);
    $txtVal = trim(preg_replace('/\s+/', '', $txtVal));
    $txtVal = "v=DKIM1; k=rsa; p={$txtVal}";

    $form['dkim']['gl_mail_record_value'] = array(
      '#title' => 'DKIM TXT record value',
      '#type' => 'textarea',
      '#description' => 'Cut and paste this into the TXT record value field at your DNS host',
      '#default_value' => $txtVal,
      '#disabled' => TRUE,
    );
  }

  $form['gb_maillist_featured'] = array(
    '#type' => 'fieldset',
    '#title' => "Featured Discussion Boards settings",
    '#description' => t("Customize the Featured Discussion Boards block"),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $board_options = array();
  $forum_term = forum_forum_load(0);
  foreach ($forum_term->forums as $tid => $forum) {
    $board_options[$tid] = $forum->name;
  }

  $form['gb_maillist_featured']['gb_maillist_featured_limit'] = array(
    '#type' => 'textfield',
    '#title' => 'Maximum number of topics to show per discussion board',
    '#default_value' => variable_get('gb_maillist_featured_limit', 5),
  );

  $form['gb_maillist_featured']['gb_maillist_featured_discussions'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => 'Show these discussion boards',
    '#options' => $board_options,
    '#default_value' => variable_get('gb_maillist_featured_discussions', 0),
  );

  return system_settings_form($form);
}

function gb_maillist_generate_keys($form, $form_values) {

  $config = array(
    "digest_alg" => "des3",
    "private_key_bits" => 1024,
    "private_key_type" => OPENSSL_KEYTYPE_RSA,
  );

  $res = openssl_pkey_new($config);
  $privKey = $pubKey = '';
  openssl_pkey_export($res, $privKey);
  $pubKey = openssl_pkey_get_details($res);
  $pubKey = $pubKey["key"];

  // put the new values into the form
  $form['dkim']['gl_mail_dkim_public_key']['#value'] = $pubKey;
  $form['dkim']['gl_mail_dkim_private_key']['#value'] = $privKey;

  return $form['dkim'];
}

function gb_maillist_settings_form_validate($form, $form_state) {
  if( !ctype_digit($form_state['values']['gb_maillist_featured_limit']) ) {
    form_set_error('gb_maillist_featured_limit', t("The limit topics number must be an integer"));
  }
}

/*
DNS Host name (TXT record name):
google._domainkey
TXT record value:
v=DKIM1; k=rsa; p=MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCZT2xxNR2fmjMSuTaoDe3vSy6CFOZnBz7pl2Kvt6RTbZHV+BFwb6p+OWNG4izWvBaNfL/jy5fJr/2G9cb418Hq6CwonwegttundbRYzvhAKuZQdN8LVgDiaIIPoP16xSSGNOnsAouOpgzbCO2j/iavFcfNxU2z5C4+aKd3naVcNQIDAQAB
*/
