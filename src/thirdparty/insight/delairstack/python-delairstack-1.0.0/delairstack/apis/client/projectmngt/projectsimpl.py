"""Module dedicated to the projects resource management.

"""
from delairstack.core.errors import QueryError
from delairstack.core.resources.resource import Resource
from ...provider import ProjectManagerAPI
from ....core.resources.projectmngt.projects import Projects
from ....core.errors import UnsupportedOperationError


class ProjectsImpl(Projects):
    """This class represents an accessor to handle project objects and actions.

    """
    _template = {}
    _hidden = ['companyId', 'deleted', 'min_status', 'missions', 'owner', 'status']
    _immutable = ['created', 'modification_date', 'modification_user',
                  'real_bbox', 'user']

    def __init__(self, project_manager_api: ProjectManagerAPI, **kwargs):
        super().__init__(provider=project_manager_api, **kwargs)

    def create(self, **kwargs):
        raise UnsupportedOperationError()

    def search(self, *, name: str):
        """Gets all projects given a name.

        Args:
            name (str): the project name / mission name

        Returns:
             [Resource] : A list of projects matching with the search criteria

        Raises:
            QueryError : if the response is not consistent
        """

        query = {
            'search': name  # search on project name, missions name or flight name
            }

        content = self._provider.search(path=self._name, query=query)

        if 'projects' not in content:
            raise QueryError('"projects" item should be in the response content')

        return [Resource(id=d['_id'], desc=d, manager=self) for d in content['projects']]
