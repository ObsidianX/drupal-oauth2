<?php

/**
 * @file
 * OAuth2.0 Client Identifer administration interface.
 */

/**
 * Administration overview for oauth2_client.
 */
function oauth2_client_admin_callback() {
  $header = array(
      array('data' => t('Application name'), 'field' => 'n.title'),
      array('data' => t('Application ID'), 'field' => 'oc.client_id'),
      array('data' => t('Application secret'), 'field' => 'oc.client_secret'),
      array('data' => t('Published'), 'field' => 'n.status'),
      array('data' => t('Updated'), 'field' => 'n.changed', 'sort' => 'desc'),
      array('data' => t('Operations'), 'colspan' => 2),
  );

  $result = db_query("SELECT oc.cid, oc.title, oc.client_id, oc.client_secret, oc.status, oc.changed FROM {oauth2_clients} AS oc ");



  $rows = array();
  foreach ($result as $record) {
    $rows[] = array(
        l($record->title, 'oauth2-client/'. $record->cid),
        $record->client_id,
        $record->client_secret,
        $record->status ? t('Yes') : t('No'),
        format_interval(time() - $record->changed),
        l(t('edit'), 'oauth2-client/'. $record->cid .'/edit', array('query' => array('destination' => 'admin/structure/oauth2/oauth2-client'))),
        l(t('delete'), 'oauth2-client/'. $record->cid .'/delete', array('query' => array('destination' => 'admin/structure/oauth2/oauth2-client'))),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => '<em>'. t('There are currently no OAuth2.0 Client Identifer.') .'</em>', 'colspan' => 7));
  }

  $output = array(
      'add_link'    => array(
          '#theme'    => 'link',
          '#text'     => 'Add Client Identifier',
          '#path'     => 'oauth2-client/add',
          '#options' => array(
              'attributes' => array('class' => array('oauth2-client-add-link'), 'id' => 'oauth2-client-add-link'),
              //REQUIRED:
              'html' => FALSE,
          ),
      ),
      'client_list' => array(
          '#header' => $header,
          '#rows'   => $rows,
          '#theme'  => 'table',
      ),
  );

  return $output;
}

/**
 * Simple page view for 1 row
 */
function oauth2_client_view_callback($oauth2_client) {
  $header = array(
      array('data' => t('Key'), 'field' => 'Value'),
      array('data' => t('Value'), 'field' => 'Value')
  );
  $rows = array();
  foreach ($oauth2_client as $key => $value) {
    if ($key == 'cid') {
      continue;
    }
    $rows[] = array($key,$value);
  }
  $output = array(
      '#header' => $header,
      '#rows' => $rows,
      '#theme' => 'table'
  );
  return $output;
}

/**
 * Add/Edit Client Identifier form
 */
function oauth2_client_edit_form($form, &$form_state, $oauth2_client = NULL) {

  $form['#oauth2_client'] = $oauth2_client;
  $form['oauth2_client'] = array(
      '#type' => 'fieldset',
      '#title' => t('OAuth2.0 settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
  );
  $form['oauth2_client']['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#required' => TRUE,
      '#default_value' => isset($oauth2_client->title) ? $oauth2_client->title : '',
      '#maxlength' => 255,
      '#weight' => -5,
  );

  // Generate a random client_id.
  if (!isset($oauth2_client->client_id) || empty($oauth2_client->client_id)) {
    $oauth2_client->client_id = md5(base64_encode(pack('N6', mt_rand(), mt_rand(), mt_rand(), mt_rand(), mt_rand(), uniqid())));
  }
  $form['oauth2_client']['client_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Application ID'),
      '#description' => t('Your OAuth2.0 <em>client_id</em>.'),
      '#value' => $oauth2_client->client_id,
      '#disabled' => TRUE,
  );

  // Generate a random client_secret.
  if (!isset($oauth2_client->client_secret) || empty($oauth2_client->client_secret)) {
    $oauth2_client->client_secret = md5(base64_encode(pack('N6', mt_rand(), mt_rand(), mt_rand(), mt_rand(), mt_rand(), uniqid())));
  }
  $form['oauth2_client']['client_secret'] = array(
      '#type' => 'textfield',
      '#title' => t('Application Secret'),
      '#description' => t('Your OAuth2.0 <em>client_secret</em>.'),
      '#value' => $oauth2_client->client_secret,
      '#disabled' => TRUE,
  );

  $form['oauth2_client']['redirect_uri'] = array(
      '#type' => 'textfield',
      '#title' => t('Site URL'),
      '#description' => t('Your site\'s URL.'),
      '#default_value' => isset($oauth2_client->redirect_uri) ? $oauth2_client->redirect_uri : '',
      '#required' => TRUE,
  );

  $form['oauth2_client']['base_domain'] = array(
      '#type' => 'textfield',
      '#title' => t('Site Domain'),
      '#description' => t('If set, Drupal will enable authentication on all subdomains (e.g., "example.com" will enable *.example.com).'),
      '#default_value' => isset($oauth2_client->base_domain) ? $oauth2_client->base_domain : '',
  );
  $form['cid'] = array(
      '#type' => 'value',
      '#value' => isset($oauth2_client->cid) ? $oauth2_client->cid : NULL,
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
 * Client Identifier form submit
 *
 * @see oauth2_client_edit_form()
 */
function oauth2_client_edit_form_submit($form, &$form_state) {
  $oauth2_client =  array(
      'client_id'         => $form_state['values']['client_id'],
      'client_secret'     => $form_state['values']['client_secret'],
      'redirect_uri'      => $form_state['values']['redirect_uri'],
      'base_domain'       => $form_state['values']['base_domain'],
      'title'             => $form_state['values']['title'],
  );
  if ($form_state['values']['cid'] != NULL) {
    $oauth2_client['cid'] = $form_state['values']['cid'];
  }
  $return_cid = oauth2_client_update($oauth2_client);
  $form_state['values']['cid'] = $return_cid;

  $form_state['redirect'] = 'oauth2-client/' . $form_state['values']['cid'] . '/edit';
  drupal_set_message($form_state['values']['title'].t(' saved.'));
}

/**
 * Confirm form for the deletion
 */
function oauth2_client_delete_form($form, &$form_state, $oauth2_client) {
  $destination = drupal_get_destination();

  //confirm_form($form, $question, $path, $description = NULL, $yes = NULL, $no = NULL, $name = 'confirm')
  return confirm_form(
  array(
      'oauth2_client' => array(
          '#type' => 'value',
          '#value' => $oauth2_client->cid,
      ),
  ),
  t('Are you sure you want to delete this client Identifier: '.$oauth2_client->title.' ?'),
  $destination['destination'],
  t('This action cannot be undone.'),
  t('Delete Client Identifier'),
  t('Cancel')
  );
}
/**
 * Confirm form submit for the deletion
 */
function oauth2_client_delete_form_submit($form, &$form_state) {
  oauth2_client_delete($form_state['values']['oauth2_client']);
  drupal_set_message(t('Client Identifier deleted'));
}