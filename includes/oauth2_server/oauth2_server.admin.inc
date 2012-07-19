<?php

/**
 * @file
 * OAuth2.0 Server Identifer administration interface.
 */

/**
 * Administration overview for oauth2_server.
 */
function oauth2_server_admin_callback() {
  $header = array(
    array('data' => t('Server Name'), 'field' => 'osrv.title'),
    array('data' => t('Server ID'), 'field' => 'osrv.server_id'),
    array('data' => t('Published'), 'field' => 'osrv.status'),
    array('data' => t('Updated'), 'field' => 'osrv.changed', 'sort' => 'desc'),
    array('data' => t('Operations'), 'colspan' => 2),
  );

  $result = db_query("SELECT osrv.sid, osrv.title, osrv.server_id, osrv.status, osrv.changed FROM {oauth2_servers} AS osrv ");
  $output = array();
  $rows = array();
  $output['add'] = array(
      '#type' => 'markup',
      '#markup' => l(t('Add server'),'oauth2-server/add'),
  );
  foreach($result as $server) {
    $output[$entity->sid] = array(
        'link' => array(
          '#type' => 'markup',
          '#markup' => l($server->title, 'oauth2-server/'. $server->sid),
        ),
        'server_id' => array(
          '#type' => 'markup',
          '#markup' => $server->server_id,
        ),
        'status' => array(
          '#type' => 'markup',
          '#markup' => $server->status ? t('Yes') : t('No'),
        ),
        'updated' => array(
          '#type' => 'markup',
          '#markup' => format_interval(time() - $server->changed),
        ),
        'edit' => array(
          '#type' => 'markup',
          '#markup' => l(t('edit'), 'oauth2-server/'. $server->sid .'/edit', array('query' => array('destination' => 'admin/build/oauth2/oauth2_server'))),
        ),
        'delete' => array(
          '#type' => 'markup',
          '#markup' => l(t('delete'), 'oauth2-server/'. $server->sid .'/delete', array('query' => array('destination' => 'admin/build/oauth2/oauth2_server'))),
        ),
    );
    /*$rows[] = array(
      l($server->title, 'oauth2-server/'. $server->sid),
      $server->server_id,
      $server->status ? t('Yes') : t('No'),
      format_interval(time() - $server->changed),
      l(t('edit'), 'oauth2-server/'. $server->sid .'/edit', array('query' => array('destination' => 'admin/build/oauth2/oauth2_server'))),
      l(t('delete'), 'oauth2-server/'. $server->sid .'/delete', array('query' => array('destination' => 'admin/build/oauth2/oauth2_server'))),
    );*/
  }

  /*if (empty($rows)) {
    $rows[] = array(array('data' => '<em>'. t('There are currently no OAuth2.0 Server Identifer.') .'</em>', 'colspan' => 6));
  }*/
  return $output;
  //return theme('table', $header, $rows);
}


/**
 * Simple page view for 1 row
 */
function oauth2_server_view_callback($oauth2_server) {
  $header = array(
   array('data' => t('Key'), 'field' => 'Value')
  );
  $rows = array();
  foreach ($oauth2_server as $key => $value) {
    $rows[] = array($key,$value);
  }
  return theme('table', $header, $rows);
}

/**
 * Edit Server Identifier
 * 
 * @see oauth2_server_edit_form_validate()
 * @see oauth2_server_edit_form_submit()
 */
function oauth2_server_edit_form($form, &$form_state, $oauth2_server = NULL) {

  $form['oauth2_server'] = array(
    '#type' => 'fieldset',
    '#title' => t('OAuth2.0 settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['oauth2_server']['server_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Server ID'),
    '#description' => t('The OAuth2.0 <em>server_id</em>.'),
    '#default_value' => isset($oauth2_server->server_id) ? $oauth2_server->server_id : '',
    '#required' => TRUE,
  );

  $form['oauth2_server']['base_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Base URI'),
    '#description' => t('The authorization server\'s HTTP base URI, typically to the front page.'),
    '#default_value' => isset($oauth2_server->base_uri) ? $oauth2_server->base_uri : '',
    '#required' => TRUE,
  );

  $form['oauth2_server']['client_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Application ID'),
    '#description' => t('Your registered OAuth2.0 <em>client_id</em>.'),
    '#default_value' => isset($oauth2_server->client_id) ? $oauth2_server->client_id : '',
    '#required' => FALSE,
  );

  $form['oauth2_server']['client_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Application Secret'),
    '#description' => t('Your registered OAuth2.0 <em>client_secret</em>.'),
    '#default_value' => isset($oauth2_server->client_secret) ? $oauth2_server->client_secret : '',
    '#required' => FALSE,
  );

  $form['oauth2_server']['authorize_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('End-user authorization endpoint'),
    '#description' => t('The authorization server\'s HTTP endpoint capable of authenticating the end-user and obtaining authorization. Absolute path or path relative to base URI.'),
    '#default_value' => isset($oauth2_server->authorize_uri) ? $oauth2_server->authorize_uri : '',
    '#required' => FALSE,
  );

  $form['oauth2_server']['access_token_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Token endpoint'),
    '#description' => t('The authorization server\'s HTTP endpoint capable of issuing tokens and refreshing expired tokens. Absolute path or path relative to base URI.'),
    '#default_value' => isset($oauth2_server->access_token_uri) ? $oauth2_server->access_token_uri : '',
    '#required' => FALSE,
  );
  
  $form['sid'] = array(
    '#type' => 'value',
    '#value' => isset($oauth2_server->sid) ? $oauth2_server->sid : NULL,
  );
  
  $form['actions'] = array(
    '#type' => 'actions',
    'save' => array(
      '#type' => 'submit',
      '#value' => t('Save settings'),
    )
  );

  return $form;
  
}

/**
 * Server Identifier form submit
 * 
 * @see oauth2_server_edit_form()
 */
function oauth2_server_edit_form_submit($form, &$form_state) {
  $oauth2_server =  array(
    'sid'               => $form_state['values']['sid'],
    'server_id'         => $form_state['values']['server_id'],
    'base_uri'          => $form_state['values']['base_uri'],
    'client_id'         => $form_state['values']['client_id'],
    'client_secret'     => $form_state['values']['client_secret'],
    'authorize_uri'     => $form_state['values']['authorize_uri'],
    'access_token_uri'  => $form_state['values']['access_token_uri'],
    
  );
  oauth2_server_update($oauth2_server);
}

/**
 * Server Identifier form validate
 * 
 * @see oauth2_server_edit_form()
 */
function oauth2_server_edit_form_validate($form, &$form_state) {

  $pattern = '/^[a-z0-9-_]{3,64}$/i';
  if (!empty($form_state['values']['server_id']) && !preg_match($pattern, $form_state['values']['server_id'])) {
    form_set_error('server_id', t('Server ID should be alphanumeric strings with hyphens and underscores, 3-64 characters long.'));
  }
  
  // Ensure no duplicate server_id.
  $result = db_result(db_query("SELECT COUNT(*) FROM {oauth2_servers} AS osrv  WHERE osrv.server_id = '%s' ", $form_state['values']['server_id']));
  if ($result != 0) {
    form_set_error('server_id', t('This Server ID had beed used by other server identifer.'));
  }
}

/**
 * Confirm form for the deletion
 */
function oauth2_server_delete_form($form, &$form_state, $oauth2_server) {
  $destination = drupal_get_destination();
  
  return confirm_form(
  array(
      'oauth2_server' => array(
          '#type' => 'value',
          '#value' => $oauth2_server['sid'],
      ),
  ),
  t('Are you sure you want to delete this Server Identifier?'),
  $destination['destination'],
  t('This action cannot be undone.'),
  t('Delete Server Identifier'),
  t('Cancel')
  );
}
/**
 * Confirm form submit for the deletetion
 */
function oauth2_server_delete_form_submit($form, &$form_state) {
  oauth2_server_delete($form_state['values']['oauth2_server']);
  drupal_set_message(t('Server Identifier deleted'));
}