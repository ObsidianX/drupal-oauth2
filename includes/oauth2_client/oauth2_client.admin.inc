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

  $query = db_select('node', 'n')->extend('TableSort');
  $query->innerJoin('oauth2_clients', 'oc', 'n.vid = oc.vid');
  $query->fields('n', array('nid', 'title', 'status', 'changed'));
  $query->fields('oc', array('client_id', 'client_secret'));
  $result = $query->execute();

  $rows = array();

  foreach ($result as $node) {
    $rows[] = array(
      l($node->title, 'node/' . $node->nid),
      $node->client_id,
      $node->client_secret,
      $node->status ? t('Yes') : t('No'),
      format_interval(REQUEST_TIME - $node->changed),
      l(t('edit'), 'node/' . $node->nid . '/edit', array('query' => array('destination' => 'admin/structure/oauth2/oauth2_client'))),
      l(t('delete'), 'node/' . $node->nid . '/delete', array('query' => array('destination' => 'admin/structure/oauth2/oauth2_client'))),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => '<em>' . t('There are currently no OAuth2.0 Client Identifer.') . '</em>', 'colspan' => 7));
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}
