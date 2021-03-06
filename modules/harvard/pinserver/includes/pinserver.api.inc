<?php

/**
 * Returns TRUE user has logged in through pinserver.
 *
 * @param int $timeout
 * - Desired maximum session length, in seconds. Use FALSE for no timeout.
 *
 * @return bool
 */
function pinserver_check_status($timeout = FALSE) {
  // Verifies that the current user has a valid token with reg_time & HUID.
  if (isset($_SESSION['pinserver']['reg_time']) && isset($_SESSION['pinserver']['huid'])) {
    // Immediately returns TRUE if there is no timeout set.
    if (!$timeout) {
      return TRUE;
    }
    // Otherwise, only returns TRUE if we are before the timeout time.
    $current_time = time();
    $timeout_time = $_SESSION['pinserver']['reg_time'] + $timeout;
    if ($current_time < $timeout_time) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * API function - Checks if user is Harvard employee based on $_SESSION variable.
 *
 * @return string or FALSE
 */
function pinserver_is_harvard_employee() {
  if (isset($_SESSION['pinserver']['harvard_employee']) && !empty($_SESSION['pinserver']['harvard_employee'])) {
    return $_SESSION['pinserver']['harvard_employee'];
  }
  return FALSE;
}

/**
 * API function - Return all session values (set after successful pinserver login)
 */
function pinserver_get_session() {
  // Returns array with pinserver session values if not empty.
  if (!empty($_SESSION['pinserver'])) {
    foreach ($_SESSION['pinserver'] as $key => $val) {
      $session_values['key'] = $val;
    }
    return $session_values;
  }
  else {
    return FALSE;
  }
}

/**
 * API function - Returns huid (set after successful pinserver login)
 */
function pinserver_get_session_huid() {
  // Returns array with pinserver session values.
  if ($_SESSION['pinserver']['huid']) {
    return $_SESSION['pinserver']['huid'];
  }
  else {
    return FALSE;
  }
}

/**
 * API function - Remove session values set after successful pinserver login
 */
function pinserver_remove_session() {
  // Remove session values.
  if (isset($_SESSION['pinserver'])) {
    unset($_SESSION['pinserver']);
  }
}
