#!/usr/bin/env python

from __future__ import print_function

import os
import re
import sys
import json
import argparse

from htm.task import AbstractAction
from htm.task import (SequentialCombination, AlternativeCombination,
                      LeafCombination, ParallelCombination)
from htm.supportive import (SupportivePOMDP, AssembleFoot, AssembleTopJoint,
                            AssembleLegToTop, BringTop, NHTMHorizon)
from htm.lib.pomdp import POMCPPolicyRunner

import rospy
from std_msgs.msg import String
from baxter_collaboration.graph_policy_controller import BaseController, DoAction
from baxter_collaboration.service_request import ServiceRequest


# Arguments
parser = argparse.ArgumentParser(
    description="Script to run users for the Tower building experiment.")
parser.add_argument(
    'path',
    help='path used for experiment, must contain pomdp.json and policy.json')
parser.add_argument('user', help='user id, used for storing times')

args = parser.parse_args(sys.argv[1:])


# Algorithm parameters
ITERATIONS = 100
EXPLORATION = 10  # 1000
RELATIVE_EXPLO = True  # In this case use smaller exploration
BELIEF_VALUES = False
N_WARMUP = 10


class UnexpectedActionFailure(RuntimeError):

    def __init__(self, arm, action, msg):
        super(UnexpectedActionFailure, self).__init__(
            'Unexpected failure for "{}" on {} arm: {}'.format(action, arm, msg))


class POMCPController(BaseController):

    OBJECTS_LEFT = "action_provider/objects_left"
    OBJECTS_RIGHT = "action_provider/objects_right"

    BRING = 'get_pass'
    CLEAR = 'cleanup'

    def __init__(self, policy, *args, **kargs):
        super(POMCPController, self).__init__(*args, **kargs)
        self.objects_left = self._parse_objects(rospy.get_param(self.OBJECTS_LEFT))
        self.objects_right = self._parse_objects(rospy.get_param(self.OBJECTS_RIGHT))
        self.pol = policy
        self.model = self.pol.model

    def _parse_objects(self, obj_dict):
        obj_parser = re.compile('(.*)_[0-9]+$')
        d = {}
        for o in obj_dict:
            new_o = obj_parser.match(o).group(1)
            if new_o not in d:
                d[new_o] = []
            d[new_o].append(obj_dict[o])
        return d

    def take_action(self, action):
        a = action.split()
        if a[0] == 'bring':
            return self.action_bring_or_clean(self.BRING, a[1])
        elif a[0] == 'clean':
            return self.action_bring_or_clean(self.CLEAN, a[1])
        elif a == 'hold':
            return self.action_hold()
        elif a == 'wait':
            raise NotImplementedError
        else:
            raise ValueError('Unknown action: {}'.format(action))

    def action_bring_or_clean(self, a, obj):
        rospy.loginfo("Action {} on {}.".format(a, obj))
        if obj in self.objects_left:
            # Note: always left if object reachable by both arms
            arm = self.action_left
            obj_ids = self.objects_left[obj]
        elif obj in self.objects_right:
            arm = self.action_right
            obj_ids = self.objects_right[obj]
        result = arm(a, obj_ids)
        if result.success:
            return self.model.O_NONE
        elif result.response == DoAction.NO_OBJ:
            return self.model.O_NOT_FOUND
        elif result.response == DoAction.ACT_FAILED:
            return self.model.O_FAIL
        else:
            raise UnexpectedActionFailure(
                'left' if arm is self.action_left else 'right', a,
                result.response)

    def action_hold(self):
        result = self.action_right(self.HOLD, [])
        if result.success:
            return self.model.O_NONE
        else:
            raise UnexpectedActionFailure('right', self.HOLD, result.response)


# Problem definition
leg_i = 'leg-{}'.format
mount_legs = SequentialCombination([
    SequentialCombination([LeafCombination(AssembleFoot(leg_i(i))),
                           LeafCombination(AssembleTopJoint(leg_i(i))),
                           LeafCombination(AssembleLegToTop(leg_i(i))),
                           ])
    for i in range(4)])
htm = SequentialCombination([LeafCombination(BringTop()), mount_legs])

p = SupportivePOMDP(htm)
pol = POMCPPolicyRunner(p, iterations=ITERATIONS,
                        horizon=NHTMHorizon.generator(p, n=3),
                        exploration=EXPLORATION,
                        relative_exploration=RELATIVE_EXPLO,
                        belief_values=BELIEF_VALUES,
                        belief='particle', belief_params={'n_particles': 100})

# Warm up policy
best = None
maxl = 0
for i in range(N_WARMUP):
    s = 'Exploring... [{:2.0f}%] (current best: {} [{:.1f}])'.format(
        i * 100. / N_WARMUP, best, pol.tree.root.children[pol._last_action].value
        if pol._last_action is not None else 0.0)
    maxl = max(maxl, len(s))
    print(' ' * maxl, end='\r')
    print(s, end='\r')
    best = pol.get_action()  # Some exploration
print('Exploring... [done]')
if BELIEF_VALUES:
    print('Found {} distinct beliefs.'.format(len(pol.tree._obs_nodes)))
dic = pol.trajectory_trees_from_starts()
dic['actions'] = pol.tree.model.actions
dic['states'] = pol.tree.model.states
dic['exploration'] = EXPLORATION
dic['relative_exploration'] = RELATIVE_EXPLO
# Store POMCP representation
with open(os.path.join(args.path, 'pomcp-{}.json'.format(args.user)), 'w') as f:
    json.dump(dic, f, indent=2)


timer_path = os.path.join(args.path, 'timer-{}.json'.format(args.user))
controller = POMCPController(pol, timer_path=timer_path)