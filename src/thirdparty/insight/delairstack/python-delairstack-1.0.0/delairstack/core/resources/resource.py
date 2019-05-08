import copy

from delairstack.core.errors import ImmutableAttribute
from delairstack.core.utils.utils import flatten_dict


class Resource(object):
    __base_immutable = ['_id', 'id', '_hidden', '_immutable']
    __base_hidden = ['_ori', '_desc', '__v']

    def __init__(self, id, *, desc, manager=None):
        self.__id = id
        self._desc = desc

        self._ori = copy.deepcopy(desc)

        hidden = getattr(manager, '_hidden', [])
        hidden += self.__base_hidden
        self._hidden = hidden

        # must be set last since it changes setattr behavior
        immutable = getattr(manager, '_immutable', [])
        immutable += self.__base_immutable
        self._immutable = immutable

    def __dir__(self):
        return list(set([x for x in ['id', *self._desc] if x not in self._hidden]))

    def __getattr__(self, name):
        if '_hidden' not in self.__dict__:
            return super().__getattr__(name)

        if name in self.__dict__['_hidden']:
            raise AttributeError(name)

        # proxy for id, _desc, _hidden and _immutable
        if name == 'id':
            return self.__id
        if name in ('_ori', '_desc', '_hidden', '_immutable'):
            if (name == '_immutable' or name == '_hidden') and name not in self.__dict__:
                return super().__getattr__(name)
            else:
                return self.__dict__[name]

        if name not in self.__dict__['_desc']:
            raise AttributeError(name)

        return self.__dict__['_desc'][name]

    def __setattr__(self, name, value):
        if '_immutable' not in self.__dict__:
            return super().__setattr__(name, value)

        if name in self._immutable:
            raise ImmutableAttribute(name)

        self._desc[name] = value

    def _diff(self):
        """
            Compute a diff between two dicts

            Returns: the changes as key paths

        """
        copy = flatten_dict(self._desc)
        ori = flatten_dict(self._ori)

        changes = []

        for v in ori:
            # added attributes
            if v not in copy:
                changes.append(v)
            # updated attributes
            elif ori[v] != copy[v]:
                changes.append(v)

        # deleted attributes
        for v in copy:
            if v not in ori:
                changes.append(v)

        return changes

    def check_attribute(self, attribute_path):
        changes = self._diff()

        for v in changes:
            if v.startswith(attribute_path):
                return True

        return False
