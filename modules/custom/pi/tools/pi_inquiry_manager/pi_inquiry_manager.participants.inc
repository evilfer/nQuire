<?php

function pi_inquiry_manager_participants($nid) {
	$inquiry = node_load($nid);
	$output = theme('pi_inquiry_manager_participants_view_inquiry_group', $inquiry);

	$output .= theme('pi_inquiry_manager_participants_list', $inquiry);

	$output .= theme('pi_inquiry_manager_participants_roles', $inquiry);

	return $output;
}

function theme_pi_inquiry_manager_participants_view_inquiry_group($inquiry_node) {

	$output = '';

	$group = NULL;
	$inquiry_selection = pi_inquiry_groupings_get_selection_type($inquiry_node);
	switch ($inquiry_selection) {
		case 'justme':
			$output .= t('This inquiry is just for myself.');
			break;
		case 'group':
			$cohort = node_load(pi_inquiry_groupings_get_cohort_group($inquiry_node));
			og_load_group($cohort);

			$output .= t('This inquiry is for members of group: <b>@group</b>.', array('@group' => $cohort->og_description));
			$group = pi_inquiry_groupings_get_inquiry_group($inquiry_node);
			break;
		case 'anyone':
			$output .= t('This inquiry is for anyone.');
			$group = pi_inquiry_groupings_get_inquiry_group($inquiry_node);
			break;
	}

	if ($group) {
		$output .= '<br/>';

		$group_selection = pi_inquiry_groupings_get_join_mode($group);
		switch ($group_selection) {
			case 0:
				$output .= $inquiry_selection === 'group' ? t('Access to the inquiry is open for group members.') : t('Access to the inquiry is open.');
				break;
			case 1:
				$output .= t('Access needs to be approved by the inquiry owner.');
				break;
			case 2:
				$output .= t('Access is through invitation only.');
				break;
		}
	}

	if (pi_inquiry_manager_participants_group_configure_inquiry_participants_access($inquiry_node->nid)) {
		$output .= '<br/>' . l(t('Edit'), "inquiry/{$inquiry_node->nid}/participants/access");
	}
	return $output;
}

function pi_inquiry_manager_participants_edit_inquiry_group($inquiry_nid) {
	$inquiry_node = node_load($inquiry_nid);
	return drupal_get_form('pi_inquiry_manager_participants_edit_inquiry_group_form', $inquiry_node);
}

function pi_inquiry_manager_participants_edit_inquiry_group_form(&$form_state, $inquiry_node) {
	$form = array();

	ahah_helper_register($form, $form_state);

	nquire_commons_load_ahah_fix();

	$current_participants_selection = $inquiry_node->participants_selection;
	$participants_selection = nquire_commons_form_node_default_value($inquiry_node, 'participants_selection', $form_state, 'target', 'justme');

	$has_changed = $current_participants_selection !== $participants_selection;

	$form['target'] = array(
			'#type' => 'select',
			'#title' => t('Who can join this inquiry'),
			'#description' => t('Select the group of users that may become participants of this inquiry.'),
			'#options' => array('justme' => t('Just me'), 'group' => t('Members of a group'), 'anyone' => t('Anyone')),
			'#default_value' => $participants_selection,
			'#ahah' => array(
					'path' => ahah_helper_path(array('target_dependent_section')),
					'wrapper' => 'target_dependent_section',
					'method' => 'replace',
					'effect' => 'fade',
			),
			'#required' => TRUE,
	);

	nquire_commons_create_form_wrapped_section($form, 'target_dependent_section');


	$participants_cohort = nquire_commons_form_node_default_value($inquiry_node, 'participants_cohort', $form_state, 'cohort_group', 0);

	if ($participants_selection === 'group') {
		$groups = array('' => t('- Select group')) + pi_inquiry_groupings_get_normal_groups();

		$form['target_dependent_section']['wrapped_content']['cohort_group'] = array(
				'#type' => 'select',
				'#title' => t('User group'),
				'#description' => t('Select the group whose participants may join this inquiry.'),
				'#options' => $groups,
				'#default_value' => $participants_cohort,
				'#required' => TRUE,
				'#ahah' => array(
						'path' => ahah_helper_path(array('target_dependent_section', 'wrapped_content', 'cohort_dependent_section')),
						'wrapper' => 'cohort_dependent_section',
						'method' => 'replace',
						'effect' => 'fade',
				),
		);

		if (!$has_changed) {
			$has_changed = $participants_cohort !== $inquiry_node->participants_cohort;
		}
	}


	_pi_inquiry_manager_participants_create_group_join_mode_form_item($form['target_dependent_section']['wrapped_content'], $form_state, $inquiry_node->actor_gid, 'join_mode', t('How to join the inquiry'), t('Select how users can join this inquiry.'), $participants_selection === 'justme');

	nquire_commons_create_form_wrapped_section($form['target_dependent_section']['wrapped_content'], 'cohort_dependent_section');
	if ($has_changed) {
		$to_be_expelled = _pi_inquiry_manager_participants_get_participants_to_be_expelled($inquiry_node, $participants_selection, $participants_cohort);

		if (count($to_be_expelled) > 0) {
			$message = '<div><p>' . t('The following users will be expelled from the inquiry:') . '</p>'
							. theme_item_list(array_map(function($uid) {
																$u = user_load($uid);
																return check_plain($u->name);
															}, $to_be_expelled))
							. '<div>';

			$form['target_dependent_section']['wrapped_content']['cohort_dependent_section']['wrapped_content'] = array(
					'#value' => $message,
			);
		}
	}


	nquire_commons_create_form_save_cancel_buttons($form, 'pi_inquiry_manager_participants_edit_inquiry_group_form_submit', "inquiry/{$inquiry_node->nid}/participants");

	return $form;
}

function _pi_inquiry_manager_participants_get_participants_to_be_expelled($inquiry_node, $new_participant_selection, $new_participant_cohort) {
	$to_be_expelled = array();
	$new_allowed_members = pi_inquiry_groupings_get_allowed_participants($new_participant_selection, $inquiry_node->uid, $new_participant_cohort);
	if ($new_allowed_members !== FALSE) {
		$current_members = pi_inquiry_groupings_get_group_member_uids($inquiry_node->actor_gid);
		foreach ($current_members as $member_uid) {
			if (!in_array($member_uid, $new_allowed_members)) {
				$to_be_expelled[] = $member_uid;
			}
		}
	}
	return $to_be_expelled;
}

function pi_inquiry_manager_participants_edit_inquiry_group_form_submit($form, $form_state) {
	$inquiry_nid = arg(1);
	if (pi_inquiry_manager_participants_group_configure_inquiry_participants_access($inquiry_nid)) {
		$inquiry = node_load($inquiry_nid);
		$new_participant_selection = $form_state['values']['target'];
		$new_participant_cohort = $form_state['values']['cohort_group'];

//$to_be_expelled = _pi_inquiry_manager_participants_get_participants_to_be_expelled($inquiry, $new_participant_selection, $new_participant_cohort);

		$inquiry->participants_selection = $new_participant_selection;
		$inquiry->participants_cohort = $new_participant_cohort;

		$inquiry->revision = TRUE;
		node_save($inquiry);


		_pi_inquiry_manager_participants_process_join_mode_form_value($inquiry->actor_gid, $form_state['values']['join_mode']);
	}

	nquire_commons_form_close($form, $form_state);
}

function _pi_inquiry_manager_participants_create_group_join_mode_form_item(&$form, $form_state, $group_nid, $id, $title, $description, $closed = FALSE) {
	if ($closed) {
		
	} else {
		$og = node_load($group_nid);
		og_load_group($og);
		$selective = nquire_commons_form_node_default_value($og, 'og_selective', $form_state, $id, '');

		if ($selective === OG_CLOSED) {
			$selective = '';
		}

		$form[$id] = array(
				'#type' => 'select',
				'#title' => $title,
				'#description' => $description,
				'#options' => array('' => t('- Select join mode'), OG_MODERATED => t('By application (approval needed)'), OG_OPEN => t('By application (automatic approval)'), OG_INVITE_ONLY => t('By invitation from owner')),
				'#default_value' => $selective,
				'#required' => TRUE,
		);
	}
}

function _pi_inquiry_manager_participants_process_join_mode_form_value($group_nid, $value) {
	nquire_group_set_join_mode($group_nid, $value);
}

/* Participant list */

function theme_pi_inquiry_manager_participants_list($inquiry_node, $group_nid = FALSE, $owner_can_leave = FALSE) {
	$gid = pi_inquiry_groupings_get_group_nid($inquiry_node, $group_nid);
	if ($gid) {
		global $user;

		$og = node_load($gid);
		og_load_group($og);

		$im_owner = $og->uid === $user->uid;

		$roles = pi_inquiry_groupings_get_inquiry_role_names($inquiry_node);
		$has_roles = count($roles) > 0;

		$members = pi_inquiry_groupings_get_group_member_uids($gid);
		$members_data = array_map(function($uid) use ($gid, $user) {
							$u = user_load($uid);
							return array(
									'uid' => $uid,
									'status' => nquire_group_get_user_status_in_group($gid, $uid),
									'name' => $user->uid === $uid ? t('Me') : check_plain($u->name),
							);
						}, $members);

		usort($members_data, function($a, $b) use($user) {
							if ($a['uid'] === $user->uid) {
								return -1;
							} else if ($b['uid'] === $user->uid) {
								return 1;
							}

							if ($a['status'] !== $b['status']) {
								if ($a['status'] === 'member') {
									return -1;
								} else if ($b['status'] === 'member') {
									return 1;
								}
							}

							return strcmp($a['name'], $b['name']);
						});

		$rows = array();

		foreach ($members_data as $member_data) {
			$status = theme('pi_inquiry_manager_participants_group_status_and_options', $inquiry_node, $member_data['uid'], $group_nid, $owner_can_leave);
			$row = array($member_data['name'], $status);

			if ($has_roles && $member_data['status'] === 'member') {
				$user_roles = nquire_group_get_user_roles($gid, $member_data['uid']);
				$user_role_list = array_map(function($role_id) use ($roles, $inquiry_node, $group_nid, $member_data, $im_owner) {
									$role_output = $roles[$role_id];
									if ($im_owner) {
										$role_output .= '<small>&nbsp;&nbsp;&nbsp;&nbsp;'
														. _pi_inquiry_manager_partipants_group_operation_link(t('remove'), $inquiry_node->nid, 'removeuserrole', $group_nid, array($member_data['uid'], $role_id))
														. '</small>';
									}
									return $role_output;
								}, $user_roles);

				$user_role_list_output = theme_item_list($user_role_list);
				if ($im_owner) {
					$user_role_list_output .= '<div><small>'
									. _pi_inquiry_manager_partipants_group_operation_link(t('Add role'), $inquiry_node->nid, 'addrolestouser', $group_nid, $member_data['uid'])
									. '</small></div>';
				}
				$row[] = $user_role_list_output;
			}

			$rows[] = $row;
		}

		$header = array(t('User'), t('Status'));
		if ($has_roles) {
			$header[] = t('Roles');
		}

		$output = nquire_commons_create_not_expanding_table_block(theme_table($header, $rows));

		if ($og->og_selective == OG_INVITE_ONLY && pi_inquiry_manager_participants_group_manageuser_access($inquiry_node->nid, $group_nid)) {
			$output .= '<p>'
							. _pi_inquiry_manager_partipants_group_operation_link(t('Invite users'), $inquiry_node->nid, 'invite', $group_nid, FALSE)
							. '</p>';
		}

		return $output;
	}
}

function _pi_inquiry_manager_partipants_group_operation_link($label, $inquiry_nid, $op, $group_nid, $elements) {
	return l($label, _pi_inquiry_manager_partipants_group_operation_path($inquiry_nid, $op, $group_nid, $elements), array('query' => drupal_get_destination()));
}

function _pi_inquiry_manager_partipants_group_operation_path($inquiry_nid, $op, $group_nid, $elements) {
	$path = "inquiry/$inquiry_nid/participants/$op";
	if ($elements) {
		if (is_array($elements)) {
			$path .= '/' . implode('/', $elements);
		} else {
			$path .= '/' . $elements;
		}
	}
	if ($group_nid) {
		$path .= '/' . $group_nid;
	}
	return $path;
}

function theme_pi_inquiry_manager_participants_group_status_and_options($inquiry_node, $uid, $group_nid = FALSE, $owner_can_leave = FALSE) {
	global $user;

	$gid = pi_inquiry_groupings_get_group_nid($inquiry_node, $group_nid);
	if ($gid) {
		$og = node_load($gid);
		og_load_group($og);

		$its_me = $uid === $user->uid;
		$im_owner = $og->uid === $user->uid;
		$its_owner = $uid === $og->uid;
		$join_mode = $og->og_selective;
		$user_status = nquire_group_get_user_status_in_group($gid, $uid);

		$status = '';
		$buttons = array();

		$path = function($op, $uid = FALSE) use ($inquiry_node, $group_nid) {
							return _pi_inquiry_manager_partipants_group_operation_path($inquiry_node->nid, $op, $group_nid, $uid);
						};

		$link_params = array('query' => drupal_get_destination());

		if ($user_status === 'member') {
			$status .= t('Active');
			if ($its_owner) {
				$status .= '&nbsp;' . t('(owner)');
			}

			if ($its_me) {
				if (!$im_owner || $owner_can_leave) {
					$buttons[] = l(t('leave'), $path('leave'), $link_params);
				}
			} else {
				if ($im_owner) {
					$buttons[] = l(t('expel'), $path('reject', $uid), $link_params);
				}
			}
		} else if ($user_status === 'awaiting') {
			if ($join_mode == OG_MODERATED) {
				$status .= t('Awaiting approval');
				if ($im_owner) {
					$buttons[] = l(t('accept'), $path('accept', $uid), $link_params);
					$buttons[] = l(t('reject'), $path('reject', $uid), $link_params);
				} else if ($its_me) {
					$buttons[] = l(t('cancel request'), $path('leave'), $link_params);
				}
			} else if ($join_mode == OG_INVITE_ONLY) {
				$status .= t('Invitation sent');
				if ($im_owner) {
					$buttons[] = l(t('cancel invitation'), $path('cancel'), $link_params);
				} else if ($its_me) {
					$buttons[] = l(t('accept'), $path('acceptinvitation'), $link_params);
					$buttons[] = l(t('decline'), $path('leave'), $link_params);
				}
			}
		} else {
			if ($its_me) {
				if ($join_mode == OG_OPEN) {
					$buttons[] = l(t('join'), $path('join'), $link_params);
				} else if ($join_mode == OG_MODERATED) {
					$buttons[] = l(t('request membership'), $path('join'), $link_params);
				}
			}
		}

		if (count($buttons) > 0) {
			$status .= '<br/><small>' . implode('&nbsp;&nbsp;&nbsp;', $buttons) . '</small>';
		}

		return $status;
	} else {
		return 'bad group!';
	}
}

/* roles list */

function theme_pi_inquiry_manager_participants_roles($inquiry_node, $group_nid = FALSE) {

	$gid = pi_inquiry_groupings_get_group_nid($inquiry_node, $group_nid);
	if ($gid) {
		$roles_data = pi_inquiry_groupings_get_inquiry_roles($inquiry_node);
		$collaboration_type = $group_nid ? 'group' : 'all';


		$phase_nids = pi_phase_load_nids_for_inquiry_and_collaboration_type($inquiry_node->nid, $collaboration_type);

		$role_phase_activities = array();
		foreach ($phase_nids as $phase_nid) {
			$phase = node_load($phase_nid);

			$activity_nids = pi_get_activities_for_phase($phase_nid);
			$activity_titles = array();
			foreach ($activity_nids as $nid) {
				$a = node_load($nid);
				$activity_titles[$nid] = check_plain($a->title);
			}

			foreach ($roles_data as $role) {
				$activities_for_role = array_filter($activity_nids, function($aid) use($role) {
									return $role['activities'][$aid] === 'edit';
								});
				if (count($activities_for_role) > 0) {
					$role_phase_activities[$role['id']][$phase_nid] = array(
							'data' => check_plain($phase->title),
							'children' => array_map(function($aid) use ($activity_titles) {
												return $activity_titles[$aid];
											}, $activities_for_role),
					);
				}
			}
		}

		$header = array(t('Role'), t('Responsible for'), t('Participants'));
		$rows = array();
		foreach ($roles_data as $role) {
			$title = $role['title'];
			$activities = theme_item_list($role_phase_activities[$role['id']]);

			$users = nquire_group_get_participants_with_role($gid, $role['id']);
			$im_manager = pi_inquiry_manager_participants_group_manageuser_access($inquiry_node->nid, $group_nid);

			$user_list = array_map(function($uid) use ($inquiry_node, $group_nid, $im_manager, $role) {
								$user = user_load($uid);
								$output = check_plain($user->name);
								if ($im_manager) {
									$output .= '<small>&nbsp;&nbsp;&nbsp;'
													. _pi_inquiry_manager_partipants_group_operation_link(t('remove'), $inquiry_node->nid, 'removeuserrole', $group_nid, array($uid, $role['id']))
													. '</small>';
								}
								return $output;
							}, $users);
			$user_html = theme_item_list($user_list);
			if ($im_manager) {
				$user_html .= _pi_inquiry_manager_partipants_group_operation_link(t('Add participant'), $inquiry_node->nid, 'adduserstorole', $group_nid, $role['id']);
			}

			$rows[] = array($title, $activities, $user_html);
		}

		return nquire_commons_create_not_expanding_table_block(theme_table($header, $rows));
	}
}

function pi_inquiry_manager_participants_group_adduserstorole($inquiry_nid, $role_id, $group_nid = FALSE) {
	$output = drupal_get_form('pi_inquiry_manager_participants_group_adduserstorole_form', $inquiry_nid, $role_id, $group_nid);
	return $output;
}

function pi_inquiry_manager_participants_group_adduserstorole_form($form_state, $inquiry_nid, $role_id, $group_nid) {
	$form = array();

	ahah_helper_register($form, $form_state);
	nquire_commons_load_ahah_fix();

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		$og = node_load($g);
		og_load_group($og);

		$text_params = array(
				'@group' => $group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title)),
				'@role' => pi_inquiry_groupings_get_inquiry_role_name($inquiry, $role_id),
		);

		$all_group_members = pi_inquiry_groupings_get_group_member_uids($gid, TRUE);
		$reject_members = nquire_group_get_participants_with_role($gid, $role_id);
		$available_members = array_filter($all_group_members, function($uid) use ($reject_members) {
							return !in_array($uid, $reject_members);
						});

		drupal_set_title(t("Add users to role '@role' in @group", $text_params));

		_pi_inquiry_manager_participants_group_create_search_users_form_item($form, $form_state, $available_members, FALSE, t('Select users to add role'), t("The selected users will be assigned role '@role'.", $text_params));
		nquire_commons_create_form_save_cancel_buttons($form, 'pi_inquiry_manager_participants_group_adduserstorole_form_submit', $_REQUEST['destination'], t('Add users'));
	}

	return $form;
}

function pi_inquiry_manager_participants_group_adduserstorole_form_submit($form, $form_state) {
	$inquiry_nid = arg(1);
	$role_id = arg(4);
	$group_nid = arg(5);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid && isset($form_state['values']['userlist'])) {
		foreach ($form_state['values']['userlist'] as $uid => $add) {
			if ($add) {
				nquire_group_add_user_role($gid, $uid, $role_id);
			}
		}
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_addrolestouser($inquiry_nid, $uid, $group_nid = FALSE) {
	$user = user_load($uid);
	if (!$user) {
		return '<p>' . t('Not a valid user.') . '</p>';
	}

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	$og = node_load($gid);
	og_load_group($og);
	if (!$gid || !$og) {
		return '<p>' . t('Not a valid inquiry/group.') . '</p>';
	}

	$text_params = array(
			'@group' => $group_nid ?
							t("group '@title'", array('@title' => $og->description)) :
							t("inquiry '@title'", array('@title' => $inquiry->title)),
			'@user' => check_plain($user->name),
	);
	if (nquire_group_get_user_status_in_group($gid, $uid) !== 'member') {
		return '<p>' . t('@user is not a member of @group.', $text_params) . '</p>';
	}

	drupal_set_title(t('Add roles to user @user within @group', $text_params));

	$all_roles = pi_inquiry_groupings_get_inquiry_role_names($inquiry);
	$user_roles = nquire_group_get_user_roles($og->nid, $user->uid);
	$available_roles = array();
	foreach ($all_roles as $rid => $title) {
		if (!in_array($rid, $user_roles)) {
			$available_roles[$rid] = $title;
		}
	}

	$output = drupal_get_form('pi_inquiry_manager_participants_group_addrolestouser_form', $available_roles);
	return $output;
}

function pi_inquiry_manager_participants_group_addrolestouser_form($form_state, $available_roles) {
	$form = array();

	if (count($available_roles) > 0) {


		$form['roles'] = array(
				'#type' => 'checkboxes',
				'#title' => t('Select roles to add'),
				'#description' => t('The selected roles will be added.'),
				'#required' => TRUE,
				'#options' => $available_roles,
		);

		nquire_commons_create_form_save_cancel_buttons($form, 'pi_inquiry_manager_participants_group_addrolestouser_form_submit', $_REQUEST['destination'], t('Add roles'));
	} else {
		$form['message'] = array(
				'#value' => '<p>' . t('No roles available for this user.') . '</p>',
		);

		nquire_commons_create_form_cancel_button($form, $_REQUEST['destination']);
	}

	return $form;
}

function pi_inquiry_manager_participants_group_addrolestouser_form_submit($form, $form_state) {
	$inquiry_nid = arg(1);
	$uid = arg(4);
	$group_nid = arg(5);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid && isset($form_state['values']['roles'])) {
		foreach ($form_state['values']['roles'] as $role_id => $add) {
			if ($add) {
				nquire_group_add_user_role($gid, $uid, $role_id);
			}
		}
	}

	drupal_goto($_REQUEST['destination']);
}

/* confirmation pages */

function pi_inquiry_manager_participants_group_join($inquiry_nid, $group_nid = FALSE) {
	global $user;

	$output = '';

	$inquiry = node_load($inquiry_nid);
	$g = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($g) {
		$og = node_load($g);
		og_load_group($og);

		$group_param = array(
				'@group' => ($group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title))
				),
		);

		$status = nquire_group_get_user_status_in_group($g, $user->uid);

		if ($status === 'member') {
			$output .= '<p>' . t('You are already a member of @group', $group_param);
		} else if ($status === 'awaiting') {
			if ($og->og_selective == OG_MODERATED) {
				$output .= '<p>' . t('You are already requested membership of @group', $group_param);
			} else {
				$output .= '<p>' . t('You have already been invited to @group', $group_param);
			}
		} else {
			if ($og->og_selective == OG_MODERATED) {
				$label = t('Request membership');
				drupal_set_title(t('Do you want to request membership for @group?', $group_param));
			} else {
				$label = t('Join');
				drupal_set_title(t('Do you want to join @group?', $group_param));
			}

			$output .= drupal_get_form('pi_inquiry_manager_participants_group_confirm_form', 'pi_inquiry_manager_participants_group_join_form_submit', $label);
		}
	}

	return $output;
}

function pi_inquiry_manager_participants_group_join_form_submit() {
	$inquiry_nid = arg(1);
	$group_nid = arg(4);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		$og = node_load($gid);
		og_load_group($og);
		global $user;
		nquire_group_add_user_to_group($gid, $user->uid, $og->og_selective == OG_OPEN);
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_acceptinvitation($inquiry_nid, $group_nid = FALSE) {
	global $user;

	$output = '';

	$inquiry = node_load($inquiry_nid);
	$g = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($g) {
		$og = node_load($g);
		og_load_group($og);

		$group_param = array(
				'@group' => ($group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title))
				),
		);

		$status = nquire_group_get_user_status_in_group($g, $user->uid);

		if ($status === 'member') {
			$output .= '<p>' . t('You are already a member of @group', $group_param);
		} else if ($status === 'none') {
			$output .= '<p>' . t('Sorry, you have no invitation for group @group', $group_param);
		} else {
			if ($og->og_selective == OG_INVITE_ONLY) {
				drupal_set_title(t('Do you want to accept the invitation to join @group?', $group_param));
				$output .= drupal_get_form('pi_inquiry_manager_participants_group_confirm_form', 'pi_inquiry_manager_participants_group_acceptinvitation_form_submit', t('Accept invitation'));
			} else {
				$output .= '<p>' . t('Invitations are not used to join @group', $group_param);
			}
		}
	}

	return $output;
}

function pi_inquiry_manager_participants_group_acceptinvitation_form_submit() {
	$inquiry_nid = arg(1);
	$group_nid = arg(4);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		global $user;
		nquire_group_add_user_to_group($gid, $user->uid, TRUE);
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_leave($inquiry_nid, $group_nid = FALSE) {
	global $user;

	$output = '';

	$inquiry = node_load($inquiry_nid);
	$g = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($g) {
		$og = node_load($g);
		og_load_group($og);

		$group_param = array(
				'@group' => ($group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title))
				),
		);

		$status = nquire_group_get_user_status_in_group($g, $user->uid);

		if ($status === 'none') {
			$output .= '<p>' . t('You are not a member of @group', $group_param) . '</p>';
		} else {
			if ($status === 'member') {
				$label = t('Leave');
				$output .= drupal_set_title(t('Do you want to leave @group?', $group_param));
			} else if ($og->og_selective == OG_MODERATED) {
				$label = t('Cancel request');
				$output .= drupal_set_title(t('Do you want to cancel your membership request from @group?', $group_param));
			} else {
				$label = t('Decline');
				$output .= drupal_set_title(t('Do you want to decline the invitation to join @group?', $group_param));
			}
			$output .= drupal_get_form('pi_inquiry_manager_participants_group_confirm_form', 'pi_inquiry_manager_participants_group_leave_form_submit', $label);
		}
	}

	return $output;
}

function pi_inquiry_manager_participants_group_leave_form_submit() {
	$inquiry_nid = arg(1);
	$group_nid = arg(4);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		global $user;
		nquire_group_delete_user_from_group($gid, $user->uid);
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_accept_user($inquiry_nid, $uid, $group_nid = FALSE) {
	$output = '';

	$inquiry = node_load($inquiry_nid);
	$g = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($g) {
		$og = node_load($g);
		og_load_group($og);
		$user = user_load($uid);

		$params = array(
				'@group' => ($group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title))
				),
				'@user' => $user->name,
		);

		$status = nquire_group_get_user_status_in_group($g, $uid);

		if ($status === 'none') {
			$output .= '<p>' . t("User '@user' has not applied for membership in @group.", $params) . '</p>';
		} else if ($status === 'member') {
			$output .= '<p>' . t("User '@user' is already a member of @group.", $params) . '</p>';
		} else {
			if ($og->og_selective == OG_MODERATED) {
				$output .= drupal_set_title(t("Do you want to accept user '@user' in @group?", $params));
				$output .= drupal_get_form('pi_inquiry_manager_participants_group_confirm_form', 'pi_inquiry_manager_participants_group_accept_user_form_submit', t('Accept'));
			} else {
				$output .= '<p>' . t("@group does not accept membership requests.", $params) . '</p>';
			}
		}
	}

	return $output;
}

function pi_inquiry_manager_participants_group_accept_user_form_submit() {
	$inquiry_nid = arg(1);
	$uid = arg(4);
	$group_nid = arg(5);


	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		if (nquire_group_get_user_status_in_group($gid, $uid) === 'awaiting') {
			nquire_group_add_user_to_group($gid, $uid, TRUE);
		}
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_reject_user($inquiry_nid, $uid, $group_nid = FALSE) {
	$output = '';

	$inquiry = node_load($inquiry_nid);
	$g = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($g) {
		$og = node_load($g);
		og_load_group($og);
		$user = user_load($uid);

		$params = array(
				'@group' => ($group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title))
				),
				'@user' => $user->name,
		);

		$status = nquire_group_get_user_status_in_group($g, $uid);


		$label = FALSE;
		if ($status === 'member') {
			$label = t('Expel');
			drupal_set_title(t("Do you want to expel user '@user' from @group?", $params));
		} else if ($status === 'awaiting') {
			if ($og->og_selective == OG_MODERATED) {
				$label = t('Reject');
				drupal_set_title(t("Do you want to reject user '@user' from @group?", $params));
			} else {
				$output .= '<p>' . t("@group does not accept membership requests.", $params) . '</p>';
			}
		} else {
			$output .= '<p>' . t("User '@user' has not applied for membership in @group.", $params) . '</p>';
		}

		if ($label) {
			$output .= drupal_get_form('pi_inquiry_manager_participants_group_confirm_form', 'pi_inquiry_manager_participants_group_reject_user_form_submit', $label);
		}
	}

	return $output;
}

function pi_inquiry_manager_participants_group_reject_user_form_submit() {
	$inquiry_nid = arg(1);
	$uid = arg(4);
	$group_nid = arg(5);


	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		nquire_group_delete_user_from_group($gid, $uid);
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_confirm_form($form_status, $save_callback, $save_label) {
	$form = array();
	nquire_commons_create_form_save_cancel_buttons($form, $save_callback, $_REQUEST['destination'], $save_label);
	return $form;
}

function _pi_inquiry_manager_participants_group_create_search_users_form_item(&$form, $form_state, $available_users, $reject_list, $list_item_title, $list_item_description) {
	$username = nquire_commons_form_default_value($form_state, 'username', '');

	$form['username'] = array(
			'#type' => 'textfield',
			'#title' => $available_users ? t('Filter user names') : t('Search user names'),
			'#description' => $available_users ? t('You can use this field to search users within this group.') : t('You can use this field to search for users in the system.'),
			'#default_value' => $username,
			'#ahah' => array(
					'path' => ahah_helper_path(array('username_dependent_section')),
					'wrapper' => 'username_dependent_section',
					'method' => 'replace',
					'event' => 'keyup',
					'keypress' => TRUE,
			),
	);

	$message = '';

	if ($available_users) {
		$user_names = array();
		foreach ($available_users as $uid) {
			$user = user_load($uid);
			$user_names[$uid] = check_plain($user->name);
		}

		$n0 = count($available_users);
		if (strlen($username) < 1) {
			$message .= t('@n users available: ', array('@n' => $n0));
		} else {
			$user_names = array_filter($user_names, function($name) use ($username) {
								return strpos($name, $username) !== FALSE;
							});
			$message .= t("@n1 users out of $n0 match '@t': ", array('@n0' => $n0, '@n1' => count($user_names), '@t' => $username));
		}
	} else { //search
		$user_names = array();
		if (strlen($username) < 2) {
			$message .= t('Please enter at least two characters in the search field.');
		} else {
			$query = "SELECT uid, name FROM {users} WHERE name LIKE '%%%s%%'";
			$result = db_query($query, $username);
			while ($row = db_fetch_array($result)) {
				if (!$reject_list || !in_array($row['uid'], $reject_list)) {
					$user_names[$row['uid']] = check_plain($row['name']);
				}
			}

			$message .= count($user_names) > 0 ? t('@n users found:', array('@n' => count($user_names))) : t("No users found for '@t'", array('@t' => $username));
		}
	}


	nquire_commons_create_form_wrapped_section($form, 'username_dependent_section');


	$form['username_dependent_section']['wrapped_content']['message'] = array(
			'#value' => '<p><i>' . $message . '</i></p>',
	);

	if ($user_names && count($user_names) > 0) {
		$form['username_dependent_section']['wrapped_content']['userlist'] = array(
				'#type' => 'checkboxes',
				'#title' => $list_item_title,
				'#description' => $list_item_description,
				'#options' => $user_names,
				'#default_value' => isset($form_state['values']['userlist']) ? $form_state['values']['userlist'] : array(),
		);
	}
}

function pi_inquiry_manager_participants_group_invite_users($inquiry_nid, $group_nid = FALSE) {
	$output = drupal_get_form('pi_inquiry_manager_participants_group_invite_users_form', $inquiry_nid, $group_nid);
	return $output;
}

function pi_inquiry_manager_participants_group_invite_users_form($form_state, $inquiry_nid, $group_nid) {
	$form = array();

	ahah_helper_register($form, $form_state);
	nquire_commons_load_ahah_fix();

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		if ($group_nid) { // invite to group
			/* TODO */
//$available = pi_inquiry_groupings_get_group_member_uids($inquiry->actor_gid);
			$og = node_load($gid);
			og_load_group($og);
			$group_name = t("group '@title'", array('@title' => $og->description));
			$available_list = FALSE;
		} else { // invite to inquiry
			$group_name = t("inquiry '@title'", array('@title' => $inquiry->title));
			$available_list = pi_inquiry_groupings_get_allowed_participants_for_inquiry($inquiry);
		}

		drupal_set_title(t('Invite users to @group', array('@group' => $group_name)));

		if ($available_list) {
			$available_list = array_filter($available_list, function($uid) use ($gid) {
								return nquire_group_get_user_status_in_group($gid, $uid) === 'none';
							});
		}
	}

	$reject_list = $available_list ? FALSE : pi_inquiry_groupings_get_group_member_uids($gid);

	_pi_inquiry_manager_participants_group_create_search_users_form_item($form, $form_state, $available_list, $reject_list, t('Select users to invite'), t("The selected users will be invited to '@group'.", array('@group' => $group_name)));

	nquire_commons_create_form_save_cancel_buttons($form, 'pi_inquiry_manager_participants_group_invite_users_form_submit', $_REQUEST['destination'], t('Invite'));
	return $form;
}

function pi_inquiry_manager_participants_group_invite_users_form_submit($form, $form_state) {
	$inquiry_nid = arg(1);
	$group_nid = arg(4);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid && isset($form_state['values']['userlist'])) {
		foreach ($form_state['values']['userlist'] as $uid => $invite) {
			if ($invite) {
				nquire_group_add_user_to_group($gid, $uid);
			}
		}
	}

	drupal_goto($_REQUEST['destination']);
}

function pi_inquiry_manager_participants_group_removeuserrole($inquiry_nid, $uid, $role_id, $group_nid = FALSE) {
	$output = '';

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		$og = node_load($gid);
		og_load_group($og);
		$user = user_load($uid);

		$params = array(
				'@group' => ($group_nid ?
								t("group '@title'", array('@title' => $og->description)) :
								t("inquiry '@title'", array('@title' => $inquiry->title))
				),
				'@user' => $user->name,
				'@role' => pi_inquiry_groupings_get_inquiry_role_name($inquiry, $role_id),
		);

		$status = nquire_group_get_user_status_in_group($gid, $uid);

		if ($status === 'none') {
			$output .= '<p>' . t("@user is not a member of @group.", $params) . '</p>';
		} else if ($status === 'awaiting') {
			$output .= '<p>' . t("@user is not yet a member of @group.", $params) . '</p>';
		} else {
			if (!nquire_group_user_has_role($gid, $uid, $role_id)) {
				$output .= '<p>' . t("@user has not role 'role' in @group.", $params) . '</p>';
			} else {
				drupal_set_title(t("Do you want to remove role '@role' for @user in @group?", $params));
				$output .= drupal_get_form('pi_inquiry_manager_participants_group_confirm_form', 'pi_inquiry_manager_participants_group_removeuserrole_form_submit', t('Remove'));
			}
		}
	}

	return $output;
}

function pi_inquiry_manager_participants_group_removeuserrole_form_submit() {
	$inquiry_nid = arg(1);
	$uid = arg(4);
	$role_id = arg(5);
	$group_nid = arg(6);

	$inquiry = node_load($inquiry_nid);
	$gid = pi_inquiry_groupings_get_group_nid($inquiry, $group_nid);
	if ($gid) {
		nquire_group_remove_user_role($gid, $uid, $role_id);
	}

	drupal_goto($_REQUEST['destination']);
}