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

  $result = db_query("SELECT osrv.sid, osrv.title, osrv.server_id, osrv.status, osrv.changed FROM {oauth2_servers} AS osrv ". tablesort_sql($header));

  $rows = array();
  while ($entity = db_fetch_object($result)) {
    $rows[] = array(
      l($entity->title, 'oauth2_server/'. $server->sid),
      $server->server_id,
      $server->status ? t('Yes') : t('No'),
      format_interval(time() - $server->changed),
      l(t('edit'), 'oauth2_server/'. $server->sid .'/edit', array('query' => array('destination' => 'admin/build/oauth2/oauth2_server'))),
      l(t('delete'), 'oauth2_server/'. $server->sid .'/delete', array('query' => array('destination' => 'admin/build/oauth2/oauth2_server'))),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => '<em>'. t('There are currently no OAuth2.0 Server Identifer.') .'</em>', 'colspan' => 6));
  }

  return theme('table', $header, $rows);
}


/**
 * Simple page view for 1 row
 */
function oauth2_server_view_callback() {
  
}