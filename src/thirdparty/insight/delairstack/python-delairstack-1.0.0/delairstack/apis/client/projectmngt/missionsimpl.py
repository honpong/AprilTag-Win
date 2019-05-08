"""Module dedicated to the mission resource management.

"""
from ....apis.provider import ProjectManagerAPI
from ....core.resources.projectmngt.missions import Missions
from ....core.resources.resource import Resource
from ....core.errors import (UnsupportedOperationError, ArgumentOperationError)


class MissionsImpl(Missions):
    """This class represents an accessor to handle mission objects and actions.

    """
    _template = {}
    _hidden = ['application', 'area', 'delivery', 'length', 'precision', 'processSettings',
               'properties', 'status', 'tags', 'workflows']
    _immutable = ['created', 'deliverables', 'modification_date', 'modification_user', 'flights', 'geometry',
                  'project', 'user']

    def __init__(self, project_manager_api: ProjectManagerAPI, **kwargs):
        super().__init__(provider=project_manager_api, **kwargs)

    def create(self, **kwargs):
        raise UnsupportedOperationError()

    def update(self, **kwargs):
        raise UnsupportedOperationError()

    def delete(self, **kwargs):
        raise UnsupportedOperationError()

    def search(self, *, project: Resource):
        """Get all missions contained in a project.

        Args:
            project (Resource): the project
            id (str): the id of the mission

        Returns:
            [Resource]: A list of missions contained in the project

        """
        missions = []
        for mission in project._desc['missions']:
            missions.append(super().get(id=mission['_id']))
        return missions
