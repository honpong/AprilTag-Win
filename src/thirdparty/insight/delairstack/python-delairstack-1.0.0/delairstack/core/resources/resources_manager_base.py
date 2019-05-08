"""Module in charge of handling CRUD actions of a Resource.

"""
from .resource import Resource
from ...apis.provider import Provider


class ResourcesManagerBase(object):
    """This class implements CRUD for any Resource.

    """
    _name = ''
    _template = {}

    def __init__(self, *, provider: Provider, **kwargs):
        self._provider = provider

    def create(self, **kwargs):
        """Create an object.

        Args:
            **kwargs:

        Returns: Resource

        """
        # Merge the template with kwargs
        # kwargs will override existing properties contained into template
        content = self._provider.post(path=self._name, data={**self._template, **kwargs})

        return Resource(id=content['_id'], desc=content, manager=self)

    def search(self, *, query):
        """Search objects matching with the query defining the criteria.

        Args:
            query: criteria to search the object

        Returns: [Resource]

        """
        content = self._provider.search(path=self._name, query=query)

        return [Resource(id=d['_id'], desc=d, manager=self) for d in content]

    def get(self, *, id: str):
        """Get an object by its id.

        Args:
            id: the id

        Returns: Resource

        """
        path = '{name}/{id}'.format(name=self._name, id=id)
        content = self._provider.get(path=path)
        return Resource(id=content['_id'], desc=content, manager=self)

    def update(self, *, resource: Resource):
        """Update the resource.

        Args:
            resource: the resource to update

        Returns: Resource

        """
        content = self._provider.put(
                path='{name}/{resource_id}'.format(name=self._name, resource_id=resource.id),
                data=resource._desc)
        # returns a new object for consistency
        return Resource(id=content['_id'], desc=content, manager=self)

    def delete(self, *, resource: Resource):
        """Delete the resource.

        Args:
            resource: the resource to delete

        Returns: True

        """
        self._provider.delete(
                path='{name}/{resource_id}'.format(name=self._name, resource_id=resource.id)
                )
        # if no exceptions is raised
        return True
