<?php

/**
 * @file
 * OAuth2.0 Scope Identifer administration interface.
 */

/**
 * Administration overview for oauth2_scope.
 */
function oauth2_scope_admin_callback() {
  $header = array(
    array('data' => t('Scope name'), 'field' => 'n.title'),
    array('data' => t('Scope ID'), 'field' => 'os.scope_id'),
    array('data' => t('Hidded'), 'field' => 'os.hidden'),
    array('data' => t('Bypass'), 'field' => 'os.bypass'),
    array('data' => t('Required'), 'field' => 'os.required'),
    array('data' => t('Published'), 'field' => 'n.status'),
    array('data' => t('Updated'), 'field' => 'n.changed', 'sort' => 'desc'),
    array('data' => t('Operations'), 'colspan' => 2),
  );

  $query = db_select('node', 'n')->extend('TableSort');
  $query->innerJoin('oauth2_scopes', 'os', 'n.vid = os.vid');
  $query->fields('n', array('nid', 'title', 'status', 'changed'));
  $query->fields('os', array('scope_id', 'hidden', 'bypass', 'required'));
  $result = $query->execute();

  $rows = array();

  foreach ($result as $node) {
    $rows[] = array(
      l($node->title, 'node/' . $node->nid),
      $node->scope_id,
      $node->hidden ? t('Yes') : t('No'),
      $node->bypass ? t('Yes') : t('No'),
      $node->required ? t('Yes') : t('No'),
      $node->status ? t('Yes') : t('No'),
      format_interval(REQUEST_TIME - $node->changed),
      l(t('edit'), 'node/' . $node->nid . '/edit', array('query' => array('destination' => 'admin/structure/oauth2/oauth2_scope'))),
      l(t('delete'), 'node/' . $node->nid . '/delete', array('query' => array('destination' => 'admin/structure/oauth2/oauth2_scope'))),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => '<em>' . t('There are currently no OAuth2.0 Scope.') . '</em>', 'colspan' => 9));
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}