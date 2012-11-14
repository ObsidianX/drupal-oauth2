<?php

/**
 * @file
 * OAuth2.0 Admin interface.
 */

function oauth2_admin_settings() {
  $form = array();
  $oauth2_mode_options = array();
  $current_provider_mode = variable_get('oauth2_provider_mode', OAUTH2_DEFAULT_PROVIDER_MODE);
  

  $form['oauth2_library_path'] = array(
    '#type' => 'textfield',
    '#title' => t('oauth2-php library path'),
    '#default_value' => variable_get('oauth2_library_path', OAUTH2_DEFAULT_LIBRARY_PATH),
    '#description' => t('The location where oauth2-php is installed. Relative paths are from the Drupal root directory.'),
    '#required' => TRUE,
    '#after_build' => array('_oauth2_admin_settings_check_library_path'),
  );


  if (module_exists('oauth2_server')) {
    $oauth2_mode_options['DrupalOAuth2Client'] = t('Client mode');
    if (module_exists('oauth2_provider') && module_exists('oauth2_consumer')) {
      $oauth2_mode_options['DrupalOAuth2Proxy'] = t('Proxy mode');
    }
  }
  if (count($oauth2_mode_options) === 1) {
    foreach ($oauth2_mode_options as $k => $v) {
      variable_set('oauth2_provider_mode', $k);
    }
  }

  $form['oauth2_provider_mode'] = array(
    '#type' => 'select',
    '#title' => t('OAuth2.0 provider mode'),
    '#default_value' => $current_provider_mode,
    '#options' => $oauth2_mode_options,
    '#description' => t('<ul><li>In proxy mode OAuth2.0 will retreive and store tokens from remote authorization server with Drupal Services 3.x API, which is useful if this server acting as resource server.</li></ul>'),
    '#required' => TRUE,
  );


  if (module_exists('oauth2_server')) {
    
    if (module_exists('oauth2_provider') && module_exists('oauth2_consumer') && $current_provider_mode == 'DrupalOAuth2Proxy') {
      $form['oauth2_provider_mode_proxy'] = array(
        '#type' => 'fieldset',
        '#title' => t('Proxy mode'),
        '#collapsible' => TRUE,
        '#collapsed' => !(variable_get('oauth2_provider_mode', OAUTH2_DEFAULT_PROVIDER_MODE) == 'DrupalOAuth2Proxy'),
      );
  
      $options = array();
      $results = oauth2_server_load_multiple();
      foreach ($results as $result) {
        $options[$result->server_id] = "{$result->title} ({$result->server_id})";
      }
      if (count($options) >= 1) {
        $form['oauth2_provider_mode_proxy']['oauth2_proxy_server_identifier'] = array(
          '#type' => 'select',
          '#title' => t('OAuth2.0 authorization server indentifier'),
          '#options' => $options,
          '#default_value' => variable_get('oauth2_proxy_server_identifier', NULL),
          '#description' => t('The OAuth2.0 authorization server indentifier. This server MUST enable Drupal Services 3.x oauth2_resource, and optionally enable oauth2_auth for additional security.'),
          '#required' => FALSE,
        );
      }
      else {
        $form['oauth2_provider_mode_proxy']['oauth2_proxy_server_identifier'] = array(
          '#value' => t("<p>To use proxy mode you should first define at least one <a href='@oauth2_server'>OAuth2.0 server indentifier</a> and <a href='@admin'>publish it</a>.</p>", array(
            '@oauth2_server' => url('node/oauth2_server/add'),
            '@admin' => url('admin/build/oauth2/oauth2_server'),
          )),
        );
      }
    }  elseif ($current_provider_mode == 'DrupalOAuth2Client') {

      $form['oauth2_provider_mode_client'] = array(
          '#type' => 'fieldset',
          '#title' => t('Client mode'),
          '#collapsible' => TRUE,
          '#collapsed' => !(variable_get('oauth2_provider_mode', OAUTH2_DEFAULT_PROVIDER_MODE) == 'DrupalOAuth2Client'),
      );
      
      $options = array();
      $results = oauth2_server_load_multiple();
      error_log(print_r($results,true));
      foreach ($results as $result) {
        $options[$result->server_id] = "{$result->title} ({$result->server_id})";
      }
      if (count($options) >= 1) {
        $form['oauth2_provider_mode_client']['oauth2_client_server_identifier'] = array(
            '#type' => 'select',
            '#title' => t('OAuth2.0 authorization server indentifier'),
            '#options' => $options,
            '#default_value' => variable_get('oauth2_client_server_identifier',NULL),
            '#description' => t('The OAuth2.0 authorization server indentifier. This server MUST enable Drupal Services 3.x oauth2_resource, and optionally enable oauth2_auth for additional security.'),
            '#required' => FALSE,
        );
      }
      else {
        $form['oauth2_provider_mode_client']['oauth2_client_server_identifier'] = array(
            '#value' => t("<p>To use client mode you should first define at least one <a href='@oauth2_server'>OAuth2.0 server indentifier</a> and <a href='@admin'>publish it</a>.</p>", array(
                '@oauth2_server' => url('oauth2-server/add'),
                '@admin' => url('admin/structure/oauth2/oauth2-server'),
            )),
        );
      }
      
      $form['oauth2_provider_mode_client']['oauth2_auto_login'] = array(
          '#type' => 'checkbox',
          '#return_value' => TRUE,
          '#title' => t('Enable auto login?'),
          '#default_value' => variable_get('oauth2_auto_login',FALSE),
          '#description' => t('User do not need to click login button if enabled. Remember to clear cache'),
          
      );
    }
  }

  $form['#validate'][] = 'oauth2_admin_settings_validate';

  return system_settings_form($form);
}

function oauth2_admin_settings_validate($form, $form_state) {
  if (module_exists('oauth2_client') && module_exists('oauth2_scope')) {
    $form_state['values']['oauth2_access_token_lifetime'] = intval($form_state['values']['oauth2_access_token_lifetime']);
    $form_state['values']['oauth2_auth_code_lifetime'] = intval($form_state['values']['oauth2_auth_code_lifetime']);
    $form_state['values']['oauth2_refresh_token_lifetime'] = intval($form_state['values']['oauth2_refresh_token_lifetime']);

    if ($form_state['values']['oauth2_access_token_lifetime'] == 0) {
      form_set_error('oauth2_access_token_lifetime', t('The access token lifetime must be a non-zero integer value.'));
    }
    if ($form_state['values']['oauth2_auth_code_lifetime'] == 0) {
        form_set_error('oauth2_auth_code_lifetime', t('The authorization code lifetime must be a non-zero integer value.'));
    }
    if ($form_state['values']['oauth2_refresh_token_lifetime'] == 0) {
      form_set_error('oauth2_refresh_token_lifetime', t('The refresh token lifetime must be a non-zero integer value.'));
    }

    if ($form_state['values']['oauth2_auth_code_lifetime'] > $form_state['values']['oauth2_access_token_lifetime']) {
      form_set_error('oauth2_auth_code_lifetime', t('The authorization code lifetime should relatively smaller than that of access token.'));
    }

    if ($form_state['values']['oauth2_refresh_token_lifetime'] < $form_state['values']['oauth2_access_token_lifetime']) {
      form_set_error('oauth2_refresh_token_lifetime', t('The refresh token lifetime should relatively larger than that of access token.'));
    }
  }
}

function _oauth2_admin_settings_check_library_path($form_element) {
  $library_path = $form_element['#value'];
  if (!is_dir($library_path) || !(file_exists($library_path .'/lib/OAuth2.inc'))) {
    form_set_error($form_element['#parents'][0], t('The oauth2-php files <em>OAuth2.inc</em> could not be found in the %library_path directory.', array('%library_path' => $library_path)));
  }
  else {
    // Flush the autoload caches so that our classes are registered.
    autoload_flush_caches();
  }
  return $form_element;
}
