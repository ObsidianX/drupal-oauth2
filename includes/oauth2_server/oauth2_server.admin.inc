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
    array('data' => t('Server Name'), 'field' => 'n.title'),
    array('data' => t('Server ID'), 'field' => 'osrv.server_id'),
    array('data' => t('Published'), 'field' => 'n.status'),
    array('data' => t('Updated'), 'field' => 'n.changed', 'sort' => 'desc'),
    array('data' => t('Operations'), 'colspan' => 2),
  );

  $query = db_select('node', 'n')->extend('TableSort');
  $query->innerJoin('oauth2_servers', 'osrv', 'n.vid = osrv.vid');
  $query->fields('n', array('nid', 'title', 'status', 'changed'));
  $query->fields('osrv', array('server_id'));
  $result = $query->execute();

  $rows = array();

  foreach ($result as $node) {
    $rows[] = array(
      l($node->title, 'node/' . $node->nid),
      $node->server_id,
      $node->status ? t('Yes') : t('No'),
      format_interval(REQUEST_TIME - $node->changed),
      l(t('edit'), 'node/' . $node->nid . '/edit', array('query' => array('destination' => 'admin/structure/oauth2/oauth2_server'))),
      l(t('delete'), 'node/' . $node->nid . '/delete', array('query' => array('destination' => 'admin/structure/oauth2/oauth2_server'))),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => '<em>' . t('There are currently no OAuth2.0 Server Identifer.') . '</em>', 'colspan' => 6));
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}
